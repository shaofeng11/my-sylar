#include "../sylar/sylar.h"
#include<time.h>
sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

void test_fiber(){
    static int s_count=5;
    SYLAR_LOG_INFO(g_logger)<<"test in fiber count="<<s_count;

    if(--s_count>=0){
       sylar::Scheduler::GetThis()->schedule(&test_fiber);// ,sylar::GetThreadId()  
    }

}

int main(int argc,char** argv){
    SYLAR_LOG_INFO(g_logger)<<"main";
    sylar::Scheduler sc(3,false,"test");
    sc.start();
    SYLAR_LOG_INFO(g_logger)<<"schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    SYLAR_LOG_INFO(g_logger)<<"end";
    // SYLAR_LOG_INFO(g_logger)<<"main";
    // sylar::Scheduler sc;
    // sc.schedule(&test_fiber);
    // sc.start();
    // sc.stop();
    // SYLAR_LOG_INFO(g_logger)<<"end";
    return 0;
}