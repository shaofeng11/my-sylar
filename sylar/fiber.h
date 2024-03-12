#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__
#include<memory>
#include"thread.h"
#include<ucontext.h>

namespace sylar{
class Scheduler;
class Fiber:public std::enable_shared_from_this<Fiber>{
friend  class Scheduler;
public:
    typedef std::shared_ptr<Fiber>ptr;
    enum State{
        // 初始化状态
        INIT,
        // 暂停状态
        HOLD,
        // 执行中状态
        EXEC,
        // 结束状态
        TERM,
        // 可执行状态
        READY,
        // 异常状态
        EXCEPT
    };


//私有方法，不允许在类外部调用,只能通过GetThis()方法，
//在返回当前正在运行的协程时，如果发现当前线程的主协程未被初始化，那就用不带参的构造函数初始化线程主协程
private:
    //当一个线程启动的时候，会自动启动一个无参的Fiber, 就是线程本身
    //每个线程中的第一个协程的构造MainFiber
    //不允许使用默认构造函数创建对象
    Fiber();
public:
    //cb 协程执行函数
    //stacksize 协程栈大小
    //带参数的构造函数用于构造子协程，初始化子协程的ucontext_t上下文和栈空间，要求必须传入协程的入口函数，以及可选的协程栈大小。
    Fiber(std::function<void()>cb,size_t stacksize=0,bool use_caller=false);
    ~Fiber();

    //重置协程函数，并重置状态
    void reset(std::function<void()>cb);

    //将当前协程切换到运行状态,将主线程切换到后台
    void swapIn();
    //切换到后台执行，切换为主线程
    void swapOut();


    void call();

    void back();

    uint64_t getId() const {return m_id;}

    State getState() const {return m_state;}
public:
    //设置当前线程的运行协程
    //f 运行协程
    static void SetThis(Fiber* f);
     
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态
    static void YieldTohold();

    //总协程数
    static uint64_t TotalFibers();

    //协程执行函数
    //执行完成返回到线程主协程
    static void MainFunc();

    static void CallerMainFunc();

    static uint64_t GetFiberId();

private:
    uint64_t m_id=0;
    // 协程运行栈大小
    uint32_t m_stacksize=0;
    State m_state=INIT;

    // 协程上下文
    ucontext_t m_ctx;

    // 协程运行栈指针
    void* m_stack=nullptr;

    //协程运行函数(真正执行的  回调)
    std::function<void()>m_cb;
};
}
#endif