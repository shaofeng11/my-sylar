#include "fiber.h"
#include<atomic>
#include"config.h"
#include"macro.h"
#include"log.h"
#include"scheduler.h"
namespace sylar{


static Logger::ptr g_logger=SYLAR_LOG_NAME("system");

//两个全局变量，所有线程共享
// 全局静态变量，用于生成协程id
static std::atomic<uint64_t>s_fiber_id{0};

//全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t>s_fiber_count{0};

//线程局部变量在每个线程都独有一份

//每个线程的当前执行协程指针，协程模块初始化时，t_fiber指向线程主协程对象。
static thread_local Fiber* t_fiber=nullptr;

//每个线程的主协程，master协程，用于切换、回收协程
//智能指针形式
static thread_local Fiber::ptr t_threadFiber=nullptr;

//创建一个配置，名称为fiber.stack_size 大小为1024 * 1024
static ConfigVar<uint32_t>::ptr g_fiber_stack_size=
    Config::Lookup<uint32_t>("fiber.stack_size",1024 * 1024,"fiber stack size");


class MallocStackAlloctor{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }
    static void Dealloc(void* vp,size_t size){
        return free(vp);
    }
};

using StackAlloctor=MallocStackAlloctor;

uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}

//无参构造函数只用于创建线程的第一个协程，也就是线程主函数对应的协程
Fiber::Fiber(){
    //主协程的状态为执行中
    m_state=EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext");
    } 
    ++s_fiber_count;
    //m_id=s_fiber_id++;
    SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber";
}

Fiber::Fiber(std::function<void()>cb,size_t stacksize,bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb){
    ++s_fiber_count;
    //如果非0直接赋值，为0  取配置里的协程栈大小
    m_stacksize=stacksize? stacksize: g_fiber_stack_size->getValue();

    m_stack=StackAlloctor::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link=nullptr;//后序上下文初始化为空
    m_ctx.uc_stack.ss_sp=m_stack;
    m_ctx.uc_stack.ss_size=m_stacksize;

   // 修改上下文信息
   //void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...)
   //参数ucp就是我们要修改的上下文信息结构体；func是上下文的入口函数；argc是入口函数的参数个数，
   //后面的…是具体的入口函数参数，该参数必须为整形值
    if(!use_caller){
        makecontext(&m_ctx,&Fiber::MainFunc,0);
    }
    else{
        makecontext(&m_ctx,&Fiber::CallerMainFunc,0);
    }

    SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber id="<<m_id;
}
Fiber::~Fiber(){
    --s_fiber_count;
    //有栈
    if(m_stack){
        //只有当结束或初始化(还未执行)或异常状态，才能释放
        SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT);
        StackAlloctor::Dealloc(m_stack,m_stacksize);
    }else{
    //无栈  说明是主协程
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state==EXEC);

        Fiber* cur=t_fiber;
        if(cur==this){
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger)<<"Fiber::~Fiber id="<<m_id;
}

//重置协程函数，并重置状态，以重复利用协程
void Fiber::reset(std::function<void()>cb){
    SYLAR_ASSERT(m_stack);//必须要有栈
    SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT);//确保状态是TERM/INIT/EXCEPT才能重置
    m_cb=cb;
    //保存、得到上下文
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext");
    }
    m_ctx.uc_link=nullptr;//后序上下文
    m_ctx.uc_stack.ss_sp=m_stack;
    m_ctx.uc_stack.ss_size=m_stacksize;            
    //修改上下文信息
    makecontext(&m_ctx,&Fiber::MainFunc,0);
    m_state=INIT;
}


void Fiber::call(){
    SetThis(this);//t_fiber=this----------t_fiber=当前协程  当前协程为rootFiber
    m_state=EXEC;
     //交换每个线程的调度协程的上下文和当前协程的上下文
    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }
}

void Fiber::back(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){
        SYLAR_ASSERT2(false,"swapconnect");
    }
}


//切换到当前协程执行
void Fiber::swapIn(){
    SetThis(this);//t_fiber=this----------t_fiber=当前协程
    //把当前协程状态改为执行
    SYLAR_ASSERT(m_state != EXEC);
    m_state=EXEC;
    //交换每个线程的调度协程的上下文和当前协程的上下文
   if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx)){//
        SYLAR_ASSERT2(false,"swapcontext");
   } 
}
//切换到后台执行
void Fiber::swapOut(){
    //将当前协程改为调度协程
    SetThis(Scheduler::GetMainFiber()); 
    //交换主协程的上下文和当前协程的上下文
    if(swapcontext(&m_ctx,&Scheduler::GetMainFiber()->m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }
}
//返回当前协程
//设置当前协程
 void Fiber::SetThis(Fiber* f){
    t_fiber=f;
 }

Fiber::ptr Fiber::GetThis(){
     if(t_fiber){
        return t_fiber->shared_from_this();
     }
     //如果当前线程没有运行着的协程（即主协程也没有），创建主协程（无参构造）
     Fiber::ptr main_fiber(new Fiber);
     SYLAR_ASSERT(t_fiber==main_fiber.get());
     t_threadFiber=main_fiber;
     return t_fiber->shared_from_this();
}
//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady(){
    Fiber::ptr cur=GetThis();//返回当前协程
    cur->m_state=READY;//当前协程状态设为READY
    cur->swapOut();//将当前协程切为后台

}
//协程切换到后台，并且设置为Hold状态
void Fiber::YieldTohold(){
    Fiber::ptr cur=GetThis();
    cur->m_state=HOLD;
    cur->swapOut();
    
}

//总协程数
uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

void Fiber::MainFunc(){
    Fiber::ptr cur=GetThis();//获得当前协程
    SYLAR_ASSERT(cur);//确保当前有协程(GteThis已经确保了cur不为空，可省略？)
    try{
        cur->m_cb();//执行回调
        cur->m_cb=nullptr;//执行完将回调清空
        cur->m_state=TERM;//结束状态
    }catch(std::exception& ex){
        cur->m_state=EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what()
                                <<" fiber_id="<<cur->getId()
                                <<std::endl
                                <<sylar::BacktraceToString();
    }catch(...){
        cur->m_state=EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except";
    }


    //t_fiber和t_thread_fiber一个是原始指针一个是智能指针，
    //混用时要注意智能指针的引用计数问题，不恰当的混用可能导致协程对象已经运行结束，但未析构问题。
    auto raw_ptr=cur.get();
    cur.reset();//智能指针重置,手动让t_fiber的引用计数减1
    raw_ptr->swapOut();//协程执行完毕时自动调用swapOut,回到主协程

    SYLAR_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
}
void Fiber::CallerMainFunc(){
    Fiber::ptr cur=GetThis();//获得当前协程
    SYLAR_ASSERT(cur);//确保当前有协程
    try{
        cur->m_cb();//执行回调
        cur->m_cb=nullptr;//执行完将回调清空
        cur->m_state=TERM;//结束状态
    }catch(std::exception& ex){
        cur->m_state=EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what()
                                <<"fiber_id="<<cur->getId()
                                <<std::endl
                                <<sylar::BacktraceToString();
    }catch(...){
        cur->m_state=EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except";
    }


    //t_fiber和t_thread_fiber一个是原始指针一个是智能指针，
    //混用时要注意智能指针的引用计数问题，不恰当的混用可能导致协程对象已经运行结束，但未析构问题。
    auto raw_ptr=cur.get();
    cur.reset();//智能指针重置,手动让t_fiber的引用计数减1
    raw_ptr->back();//协程执行完毕时自动调用swapOut,回到主协程

    SYLAR_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
}
}