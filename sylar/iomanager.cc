#include "iomanager.h"
#include<sys/epoll.h>
#include"macro.h"
#include"log.h"
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

namespace sylar{

static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

 IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event){
    switch(event){
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false,"getContext");
    }
}
void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler=nullptr;
    ctx.fiber.reset();
    ctx.cb=nullptr;
}
void IOManager::FdContext::triggerEvent(Event event){
    SYLAR_ASSERT(events & event );
    events=(Event)(events & ~event);//这样操作可以将触发过的事件删除  妙
    EventContext& ctx=getContext(event);
    if(ctx.cb){
        //用引用的方式传递，在scheduler中，调用
        // FiberAndThread(std::function<void()>*f,int thr)
        //     :thread(thr){
        //     cb.swap(*f);
        // }
        //构造函数
        ctx.scheduler->schedule(&ctx.cb);
    }else{
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler=nullptr;
    return;
}

IOManager::IOManager(size_t threads,bool use_caller,const std::string& name)
    :Scheduler(threads,use_caller,name){
    m_epfd=epoll_create(5000);
    SYLAR_ASSERT(m_epfd>0);

    //创建pipe，获取m_tickleFds[2]，其中m_tickleFds[0]是管道的读端，m_tickleFds[1]是管道的写端
    int rt=pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);
    //epoll_event事件
    epoll_event event;
    memset(&event,0,sizeof(epoll_event));
    //设置为ET边缘触发模式
    event.events=EPOLLIN|EPOLLET;

    event.data.fd=m_tickleFds[0];
    //设置非阻塞
    rt=fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);
    SYLAR_ASSERT(rt==0);

 // 将管道的读描述符加入epoll多路复用，如果管道可读，idle中的epoll_wait会返回
    rt=epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);
    SYLAR_ASSERT(rt==0);

 // 这里直接开启了Schedluer，也就是说IOManager创建即可调度协程
    contextResize(32);
    start();

}
IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i=0;i<m_fdContexts.size();++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size){
    m_fdContexts.resize(size);

    for(size_t i=0;i<m_fdContexts.size();++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i]=new FdContext;
            m_fdContexts[i]->fd=i;
        }
    }
}

//fd描述符发生了event事件时执行cb函数
int IOManager::addEvent(int fd,Event event,std::function<void()>cb){
    //找到fd对应的FdContext，如果不存在，那就分配一个
    FdContext* fd_ctx=nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()>fd){
        fd_ctx=m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd*1.5);
        fd_ctx=m_fdContexts[fd];
    }

    //同一个fd不允许重复添加相同的事件
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events&event){//fd_ctx->events==event
        SYLAR_LOG_ERROR(g_logger)<<"addEvent assert fd="<<fd
                                <<" event="<<event
                                <<" fd_ctx.event="<<fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events&event));
    }   

    // 将新的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置
    int op=fd_ctx->events ? EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events=EPOLLET|fd_ctx->events|event;
    epevent.data.ptr=fd_ctx;

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<", "
                <<op<<", "<<fd<<", "<<epevent.events<<"):"
                <<rt<<"("<<errno<<")"<<strerror(errno)<<" )";
        return -1;
    }
    ++m_pendingEventCount;

 // 找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进行赋值
    fd_ctx->events=(Event)(fd_ctx->events|event);
    FdContext::EventContext& event_ctx=fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler
                &&!event_ctx.fiber
                &&!event_ctx.cb);//确保这三个都为nullptr

    // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
    event_ctx.scheduler=Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    //如果没有回调，将当前协程作为回调
    }else{
        event_ctx.fiber=Fiber::GetThis();
        SYLAR_ASSERT(event_ctx.fiber->getState()==Fiber::EXEC);
    }
    SYLAR_LOG_INFO(g_logger)<<"add success";
    return 0;
}

bool IOManager::delEvent(int fd,Event event){
    // 找到fd对应的FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if((fd_ctx->events&event)!=0){//如果已存事件和要删除事件没有交集，返回false
        return false;
    }

 // 清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从epoll_wait中删除该文件描述符
    Event new_events=(Event)(fd_ctx->events&event);
    int op=new_events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;    
    epoll_event epevent;
    epevent.events=EPOLLET|new_events;
    epevent.data.ptr=fd_ctx;

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","
                <<op<<","<<fd<<","<<epevent.events<<"):"
                <<rt<<"("<<errno<<")"<<strerror(errno)<<")";
            return false;
    }
    --m_pendingEventCount;
    //重置该fd对应的event事件上下文(清空)
    fd_ctx->events=new_events;
    FdContext::EventContext& event_ctx=fd_ctx->getContext(event);
    //如果是要删除的是读事件，就将读事件的scheduler，cb,fiber设为空，如果是要删除的是写事件，就将写事件的scheduler,cb,fiber设为空
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd,Event event){
   RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events&event)){
        return false;
    }

    Event new_events=(Event)(fd_ctx->events&event);
    int op=new_events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;    
    epoll_event epevent;
    epevent.events=EPOLLET|new_events;
    epevent.data.ptr=fd_ctx;

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","
                <<op<<","<<fd<<","<<epevent.events<<"):"
                <<rt<<"("<<errno<<")"<<strerror(errno)<<")";
            return false;
    }
    //上面代码与删除相同
    // 删除之前触发一次事件
    fd_ctx->triggerEvent(event);
    // 活跃事件数减1
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events){//为空NONE
        return false;
    }
//fd_ctx->events不为空
// 删除全部事件
    int op=EPOLL_CTL_DEL;    
    epoll_event epevent;
    epevent.events=0;
    epevent.data.ptr=fd_ctx;

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","
                <<op<<","<<fd<<","<<epevent.events<<"):"
                <<rt<<"("<<errno<<")"<<strerror(errno)<<")";
            return false;
    }

     // 触发全部已注册的事件
    if(fd_ctx->events&READ){//fd_ctx->events==READ
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events&WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    SYLAR_ASSERT(fd_ctx->events==0);
    return true;
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

//写pipe让idle协程从epoll_wait退出，待idle协程yield之后Scheduler::run就可以调度其他任务
 //如果当前没有空闲调度线程，那就没必要发通知
void IOManager::tickle(){
    if(!hasIdThreads()){
        return;
    }
    int rt=write(m_tickleFds[1],"T",1);
    SYLAR_ASSERT(rt==1);
}

bool IOManager::stopping(){
    uint64_t timeout=0;
    return  stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout){
    timeout=getNextTime();
    return timeout==~0ull
            &&m_pendingEventCount==0
            &&Scheduler::stopping();
}

/**
 * @brief idle协程
 * @details 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
 * 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事，一是有没有新的调度任务，对应Schduler::schedule()，
 * 如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行
 * IO事件对应的回调函数
 */
void IOManager::idle(){
    //堆区内存
    epoll_event* events=new epoll_event[64]();
    //智能指针的析构函数，用来释放new出来的内存，当函数结束时释放events
    std::shared_ptr<epoll_event>shared_events(events,[](epoll_event* ptr){
        delete[] ptr;
    });
    while(true){
        uint64_t next_timeout=0;
        if(stopping(next_timeout)){
              SYLAR_LOG_INFO(g_logger)<<"name="<<getName()<<"idle stopping exit";
              break;          
        }
        int rt=0;
        do{
            //表示在没有检测到事件发生时最多等待的时间
            static const int MAX_TIMEOUT=3000;
            if(next_timeout!=~0ull){
                next_timeout=(int)next_timeout>MAX_TIMEOUT? MAX_TIMEOUT:next_timeout;
            }else{
                next_timeout=MAX_TIMEOUT;
            }
            // 阻塞在epoll_wait上，等待事件发生
            rt=epoll_wait(m_epfd,events,64,(int)next_timeout);
            if(rt<0&&errno==EINTR){
            }else{
                break;   
            }
         }while(true);

         std::vector<std::function<void()>>cbs;
         listExpiredCb(cbs);
         if(!cbs.empty()){
            schedule(cbs.begin(),cbs.end());
            cbs.clear();
         }
        //遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for(int i=0;i<rt;++i){
            epoll_event& event=events[i];
            //如果有事件触发，将tickle的那一个字符读出来
            // ticklefd[0]用于通知协程调度，这时只需要把管道里的内容读完即可，本轮idle结束Scheduler::run会重新执行协程调度
            if(event.data.fd==m_tickleFds[0]){
                uint8_t dummy[256];
                //将数据读完
                while(read(m_tickleFds[0],&dummy,sizeof(dummy))>0);
                continue;
            }
            FdContext* fd_ctx=(FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            //EPOLLERR: 出错，比如写读端已经关闭的pipe
            //EPOLLHUP: 套接字对端关闭
            //出现这两种事件，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
            if(event.events & (EPOLLERR|EPOLLHUP)){
                event.events|=(EPOLLIN|EPOLLOUT) & fd_ctx->events;
            }
            int real_events=NONE;
            if(event.events & EPOLLIN){
                real_events|=READ;
            }
            if(event.events & EPOLLOUT){
                real_events|=WRITE;
            }
            //有无交集，若无交集，跳过本次循环
            if((fd_ctx->events&real_events)==NONE){
                continue;
            }
            // 剔除已经发生的事件，将剩下的事件重新加入epoll_wait，
            // 如果剩下的事件为0，表示这个fd已经不需要关注了，直接从epoll中删除
            //剩余的事件=当前事件-正在触发的事件
            int left_events=(fd_ctx->events &~real_events);
            int op=left_events? EPOLL_CTL_MOD:EPOLL_CTL_DEL;
            event.events=EPOLLET|left_events;

            //把剩余事件重新添加到epollwait中
            int rt2=epoll_ctl(m_epfd,op,fd_ctx->fd,&event);
            if(rt2){
                SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","
                    <<op<<","<<fd_ctx->fd<<","<<event.events<<"):"
                    <<rt2<<"("<<errno<<")"<<strerror(errno)<<")";
                continue;
            }
            if(real_events&READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events&WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
    }
    /**
     * 一旦处理完所有的事件，idle协程yield，这样可以让调度协程(Scheduler::run)重新检查是否有新任务要调度
     * 上面triggerEvent实际也只是把对应的fiber重新加入调度，要执行的话还要等idle协程退出
     */
    Fiber::ptr cur=Fiber::GetThis();
    //shared_ptr中的get函数 在shared_ptr中使用get函数可以获取该shared_ptr所管理对象的原始指针。
    auto raw_ptr=cur.get();
    cur.reset();//将指针的引用计数-1
    raw_ptr->swapOut();
    }
    
}

void IOManager::onTimerInsertedAtFront(){
    tickle();
}

}