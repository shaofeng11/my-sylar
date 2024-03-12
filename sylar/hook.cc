#include"hook.h"
#include"fiber.h"
#include"log.h"
#include"iomanager.h"
#include"config.h"
#include <dlfcn.h>
#include"fd_manager.h"
static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");
namespace sylar{

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout=
    sylar::Config::Lookup("tcp.connect.timeout",5000,"tcp connect timeout");

static thread_local bool t_hook_enable=false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt) 

void hook_init(){
    static bool is_inited=false;
    if(is_inited){
        return;
    }
    //dlsym获取各个被hook的接口的原始地址
    //sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep"); 
    //usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
    //dlsym:从系统中取出名为name的函数
    #define XX(name) name ## _f=(name ## _fun)dlsym(RTLD_NEXT,#name);
        HOOK_FUN(XX);
    #undef XX
}


static uint64_t s_connect_timeout=-1;
struct  _HookIniter{
    _HookIniter(){
        //hook_init() 放在一个静态对象的构造函数中调用，
        //这表示在main函数运行之前就会获取各个符号的地址并保存在全局变量中。
        hook_init();
        s_connect_timeout=g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int old_value,const int& new_value){
                SYLAR_LOG_INFO(g_logger)<<"tcp connect timeout cahnged from"
                                           <<old_value<<"to"<<new_value;
                s_connect_timeout=new_value;
        });
    }
};

//实例化一个全局静态变量
static _HookIniter s_hook_initer;

bool is_hook_enable(){
    return t_hook_enable;
}
void set_hook_enable(bool flag){
    t_hook_enable=flag;
}
}

struct timer_info{
    int cancelled=0;
};


template<typename OriginFun,typename ... Args>
//fd,函数，函数名，SO_RCVTIMEO或SO_SNDTIMEO，时长，参数
static ssize_t do_io(int fd,OriginFun fun,const char* hook_fun_name,
                    uint32_t event,int timeout_so,Args&&... args){
    //不使用hook
    if(!sylar::t_hook_enable){
        //？？原来系统函数    forward？
        return fun(fd,std::forward<Args>(args)...);
    }
    //SYLAR_LOG_DEBUG(g_logger)<<"do_io<"<<hook_fun_name<<">";
    //得到fd的上线文信息
    sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
    //若为空，则使用原来的系统函数
    if(!ctx){
        return fun(fd,std::forward<Args>(args)...);
    }
    //如果fd关闭了，返回-1
    if(ctx->isClose()){
        //EBADF（错误的文件数)  The argument s is an invalid descriptor
        errno=EBADF;
        return -1;
    }
    //如果不是socket或是用户设置nonblock，使用原来的系统函数
    if(!ctx->isSocket()||ctx->getUserNonblock()){
        return fun(fd,std::forward<Args>(args)...);
    }

//开始hook

    //根据类型取出超时时长
    uint64_t to=ctx->getTimeout(timeout_so);

    //创建超时条件结构体
    std::shared_ptr<timer_info>tinfo(new timer_info);
   
retry://执行原始系统函数

    ssize_t n=fun(fd,std::forward<Args>(args)...);
    //SYLAR_LOG_DEBUG(g_logger)<<"do_io<"<<hook_fun_name<<">"<<" errno="<<errno;
    //Interrupted system call 信号中断
    while(n==-1&&errno==EINTR){
        n=fun(fd,std::forward<Args>(args)...);
    }
    //异步操作      阻塞状态
    if(n==-1&&errno==EAGAIN){
        //SYLAR_LOG_DEBUG(g_logger)<<"do_io<"<<hook_fun_name<<">";
        //拿出当前线程所在的iomanager
        sylar::IOManager* iom=sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;
        std::weak_ptr<timer_info>winfo(tinfo);

        //如果超时时长不为-1
        if(to!=(uint64_t)-1){
            //添加一个条件定时器
            //[]表示lanmb可以使用的所在函数的参数
            //& 或 = 向编译器声明采用引用捕获或者值捕获。编译器会将外部变量全部捕获。

            //如果等了to时长还未读到数据，触发回调
            timer=iom->addConditionTimer(to,[winfo,fd,iom,event](){
                //智能指针的lock  ->拿出这个条件
                auto t=winfo.lock();//t是一个shared_ptr
                //如果条件没有了或者条件被取消了
                if(!t||t->cancelled){
                    return;
                }
                t->cancelled=ETIMEDOUT;
                iom->cancelEvent(fd,(sylar::IOManager::Event)(event));
            },winfo);
        }
        //默认采用当前协程作为唤醒对象加入到iom中
        int rt=iom->addEvent(fd,(sylar::IOManager::Event)(event));
        //添加失败
        if(rt){
            SYLAR_LOG_ERROR(g_logger)<<hook_fun_name<<"addEvent("
                            <<fd<<","<<event<<")";
            if(timer){
                timer->cancel();
            }
            return -1;
        }else{
            //有两种方式唤醒
            //1.读到事件
            //2.定时器超时
            sylar::Fiber::YieldTohold();
            SYLAR_LOG_DEBUG(g_logger)<<"do_io<"<<hook_fun_name<<">";
            //如果是读到事件，那此时定时器用不上了，直接取消掉
            if(timer){
                timer->cancel();
            }
            //如果定时器超时了，则在回调函数中设置了tinfo的cancelled
            if(tinfo->cancelled){
                errno=tinfo->cancelled;
                return -1;
            }
            //有io事件回来,重新读
            goto retry;
        }
    }
    return n;
}


//做函数声明，在hook_init里实现
extern "C"{
    //sleep_fun sleep_f=nullptr;
    //usleep_fun usleep_f = nullptr; 
#define XX(name) name ## _fun name ## _f=nullptr; 
    HOOK_FUN(XX)
#undef XX 



//重新定义sleep
unsigned int sleep(unsigned int seconds){
    //如果不用hook,就用原来的sleep（sleep_f）
    if(!sylar::t_hook_enable){
        return sleep_f(seconds);
    }
    //创建一个协程，一个协程调度器，向协程调度器中添加一个时长为seconds*1000的定时器,然后让出协程
    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    //在iomanager中添加定时器，second秒之后唤醒，添加过后不会阻塞等待，会让出协程
    iom->addTimer(seconds *1000,std::bind((void(sylar::Scheduler::*)
                    (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
                    ,iom,fiber,-1));
    // iom->addTimer(seconds *1000,[iom,fiber](){
    //     iom->schedule(fiber);
    // });
    sylar::Fiber::YieldTohold();
    return 0;
}
int usleep(useconds_t usec){
    if(!sylar::t_hook_enable){
            return usleep_f(usec);
        }
    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    iom->addTimer(usec /1000,std::bind((void(sylar::Scheduler::*)
                    (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
                    ,iom,fiber,-1));
    // iom->addTimer(usec/1000,[iom,fiber](){
    //     iom->schedule(fiber);
    // });
    sylar::Fiber::YieldTohold();
    return 0;
}
int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!sylar::t_hook_enable){
            return nanosleep_f(req,rem);
    }
    int timeout_ms=req->tv_sec*1000+req->tv_nsec/1000/1000;
    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    //iom->addTimer(usec/1000,std::bind(&sylar::IOManager::schedule,iom,fiber));
    iom->addTimer(timeout_ms/1000,[iom,fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldTohold();
    return 0;
}

//socket
int socket(int domain, int type, int protocol){
    if(!sylar::t_hook_enable){
        return socket_f(domain,type,protocol);
    }
    int fd=socket_f(domain,type,protocol);
    if(fd==-1){
        return fd;
    }
    //将创建好的fd放入FdManager中
    sylar::FdMgr::GetInstance()->get(fd,true);
    return fd;
}

int connect_with_timeout(int fd,const struct sockaddr* addr,socklen_t addrlen, uint64_t timeout_ms){
    if(!sylar::t_hook_enable){
        return connect_f(fd,addr,addrlen);
    }
    sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx||ctx->isClose()){
        errno=EBADF;
        return -1;
    }
    if(!ctx->isSocket()){
        return connect_f(fd,addr,addrlen);
    }
    if(ctx->getUserNonblock()){
        return connect_f(fd,addr,addrlen);
    }
    int n=connect_f(fd,addr,addrlen);
    if(n==0){
        return 0;
        //EINPROGRESS代表连接还在进行中
    }else if(n!=-1||errno!=EINPROGRESS){
        return n;
    }
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info>tinfo(new timer_info);
    std::weak_ptr<timer_info>winfo(tinfo);
    if(timeout_ms!=(uint64_t)-1){
        timer=iom->addConditionTimer(timeout_ms,[winfo,fd,iom](){
            auto t=winfo.lock();
            if(!t||t->cancelled){
                return;
            }
            t->cancelled=ETIMEDOUT;
            iom->cancelEvent(fd,sylar::IOManager::WRITE);
        },winfo);
    }
    int rt=iom->addEvent(fd,sylar::IOManager::WRITE);
    if(rt==0){
        sylar::Fiber::YieldTohold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno=tinfo->cancelled;
            return -1;
        }
    }else{
        //失败
        if(timer){
            timer->cancel();
        }
        SYLAR_LOG_ERROR(g_logger)<<"connect addEvent("<<fd<<", WRITE) error";
    }

    int error=0;
    socklen_t len=sizeof(int);
    //当进程调用read且没有数据返回时，如果so_error为非0值，则read返回-1且errno设为so_error的值，接着so_error的值被复位为0。
    if(-1==getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)){
        return -1;
    }
    if(!error){
        return 0;
    }else{
        errno=error;
        return -1;
    }
}

//connect
int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen){
    return connect_with_timeout(sockfd,addr,addrlen,sylar::s_connect_timeout);
}

//accept
int accept(int s, struct sockaddr *addr, socklen_t *addrlen){
   int fd=do_io(s,accept_f,"accept",sylar::IOManager::READ,SO_RCVTIMEO,addr,addrlen);
   //将创建好的fd放入FdManager中
   if(fd>=0){
        sylar::FdMgr::GetInstance()->get(fd,true);
   }
   return fd;
}

//read
ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd,read_f,"read",sylar::IOManager::READ,SO_RCVTIMEO,buf,count);
}
//readv
ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd,readv_f,"readv",sylar::IOManager::READ,SO_RCVTIMEO,iov,iovcnt);
}
//recv
ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(sockfd,recv_f,"recv",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags);
}

//recvfrom
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd,recvfrom_f,"recvfrom",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
}

//recvmsg
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd,recvmsg_f,"recvmsg",sylar::IOManager::READ,SO_RCVTIMEO,msg,flags);
}

//write
ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd,write_f,"write",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,count);
}

//writev
ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd,writev_f,"writev",sylar::IOManager::WRITE,SO_SNDTIMEO,iov,iovcnt);
}

//send
ssize_t send(int s, const void *msg, size_t len, int flags){
   return do_io(s,send_f,"send",sylar::IOManager::WRITE,SO_SNDTIMEO,msg,len,flags);
}

//sendto
ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
    return do_io(s,sendto_f,"sendto",sylar::IOManager::WRITE,SO_SNDTIMEO,msg,len,flags,to,tolen);
}

//sendmsg
ssize_t sendmsg(int s, const struct msghdr *msg, int flags){
    return do_io(s,sendmsg_f,"sendmsg",sylar::IOManager::WRITE,SO_SNDTIMEO,msg,flags);
}

//close
int close(int fd){
    if(!sylar::t_hook_enable){
        return close_f(fd);
    }
    sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
    //先把事件取消掉，再关闭
    if(ctx){
        auto iom=sylar::IOManager::GetThis();
        if(iom){
            iom->cancelAll(fd);
        }
        sylar::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

//fcntl
int fcntl(int fd, int cmd, ... /* arg */ ){
    va_list va;
    va_start(va,cmd);
    switch (cmd){
//设置阻塞非阻塞
        case F_SETFL:   
        {
            int arg=va_arg(va,int);
            va_end(va);
            sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
            if(!ctx||ctx->isClose()||!ctx->isSocket()){
                return fcntl_f(fd,cmd,arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if(ctx->getSysNonblock()){
                arg |=O_NONBLOCK;
            }else{
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd,cmd,arg);
        }
            break; 
        case F_GETFL:
        {
            va_end(va);
            int arg=fcntl_f(fd,cmd);
            sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
            if(!ctx||ctx->isClose()||!ctx->isSocket()){
                return arg;
            }
            if(ctx->getUserNonblock()){
                return arg | O_NONBLOCK;
            }else{
                return arg & ~O_NONBLOCK;
            }
        }
        break;
//int
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
   
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ:
        {
            int arg=va_arg(va,int);
            va_end(va);
           
            return fcntl_f(fd,cmd,arg);
        }
        break;
//void
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ:
        {
            va_end(va);
            return fcntl_f(fd,cmd);
        }
        break;

//lock
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
        {
            struct  flock* arg=va_arg(va,struct  flock*);
            va_end(va);
            return fcntl_f(fd,cmd,arg);
        }
//f_owner_exlock
    case F_GETOWN_EX:
    case F_SETOWN_EX:
       {
            struct f_owner_exlock* arg=va_arg(va,struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd,cmd,arg);
       }
        break;
    default:
            va_end(va);
            return fcntl_f(fd,cmd);
    }
}

//ioctl
int ioctl(int d, unsigned long int request, ...){
    va_list va;
    va_start(va,request);
    void* arg=va_arg(va,void*);
    va_end(va);
    //如果某个套接字的FIONBIO属性设置为true那么被意味着将此套接字设置为非阻塞模式，反之则为阻塞模式。
    if(FIONBIO==request){
        bool user_nonblock=!!*(int*)arg;
        sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(d);
        if(!ctx||ctx->isClose()||!ctx->isSocket()){
            return ioctl_f(d,request,arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d,request,arg);
}

//getsockopt
int getsockopt(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd,level,optname,optval,optlen);
}

//setsockopt
int setsockopt(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen){
    if(!sylar::t_hook_enable){
        return setsockopt_f(sockfd,level,optname,optval,optlen);
    }

    if(level==SOL_SOCKET){
        //更新FdManager中的超时时间
        if(optname==SO_RCVTIMEO||optname==SO_SNDTIMEO){
            sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(sockfd);
            if(ctx){
                const timeval* v=(const timeval*)optval;
                ctx->setTimeout(optname,v->tv_sec*1000+v->tv_usec/1000);
            }
        }
    }
    return setsockopt_f(sockfd,level,optname,optval,optlen);
}

 }   



