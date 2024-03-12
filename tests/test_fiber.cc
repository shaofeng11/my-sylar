#include "../sylar/sylar.h"

sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

//子协程的回调函数
void run_in_fiber(){
    SYLAR_LOG_INFO(g_logger)<<"run_in_fiber begin";
    //sylar::Fiber::GetThis()->swapOut();

    //切换到主协程
    sylar::Fiber::YieldTohold();
    SYLAR_LOG_INFO(g_logger)<<"run_in_fiber end";
    sylar::Fiber::YieldTohold();
}

void test_fiber(){
SYLAR_LOG_INFO(g_logger)<<"main begin -1";
    {
        sylar::Fiber::GetThis();//创建主协程
        SYLAR_LOG_INFO(g_logger)<<"main begin";
        //主协程中创建子协程   回调函数为run_in_fiber，栈大小为1M
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        //将当前协程切换到运行状态,将主线程切换到后台
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger)<<"main after swapin";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger)<<"main after end";
        fiber->swapIn();
    }
    SYLAR_LOG_INFO(g_logger)<<"main after end2";
}

int main(int argc,char** argv){
    sylar::Thread::SetName("main");

    std::vector<sylar::Thread::ptr>thrs;
    for(int i=0;i<3;i++){
        thrs.push_back(sylar::Thread::ptr(
            new sylar::Thread(&test_fiber,"name_"+std::to_string(i))));
    }

    for(auto i:thrs){
        i->join();
    }
    test_fiber();
    
    return 0;
}