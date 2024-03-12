#include "thread.h"
#include "log.h"
#include "util.h"
namespace sylar
{
    //thread_local就是达到线程内独有的全局变量,线程1与线程2不同
    //thread_local声明的变量存储在线程开始时分配，线程结束时销毁。每一个thread_local变量在线程中有独立实体,各变量间互相不干扰。
    //和class类有关在线程结束时，会调用class的析构函数。
    //当前正在执行的线程
static thread_local Thread* t_thread=nullptr;
    //当前正在执行的线程名
static thread_local std::string t_thread_name="UNKNOW";


static sylar::Logger::ptr g_logger= SYLAR_LOG_NAME("system");


 Thread* Thread::GetThis(){
    return t_thread;
}

Thread::Thread(std::function<void()>cb,const std::string& name)
            :m_cb(cb)
            ,m_name(name){
    //如果传入名字为空，默认为UNKNOW
    if(name.empty()){
        m_name="UNKNOW";
    }
    //create线程，新线程的主函数为run。参数为this
    int rt=pthread_create(&m_thread,nullptr,&Thread::run,this);
    if(rt){
        //出错处理
        SYLAR_LOG_ERROR(g_logger)<<"pthread_create thread fail,rt="<<rt
                                <<"name="<<name;
        throw std::logic_error("pthread_create error");
    }

    //新创建的线程可能在父线程执析构时还未执行，需要让父线程阻塞等待，直到新创建的线程进入run后唤醒
    //可以保证线程创建成功后一定是跑起来的
    m_semaphore.wait();
}

 void* Thread::run(void* arg){
    Thread* thread=(Thread*)arg;
    //
    t_thread=thread;
    t_thread_name=thread->m_name;
    thread->m_id=sylar::GetThreadId();
    //给线程起一个名字，最大长度为16，默认为"UNKNOW"
    pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());

    std::function<void()>cb;
    cb.swap(thread->m_cb);

    //唤醒父线程
    thread->m_semaphore.notify();
    cb();
    return 0;
 }
Thread:: ~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }
}

 const std::string& Thread::GetName(){
    return t_thread_name;
 }

void Thread::SetName(const std::string& name){
    if(t_thread){
        t_thread->m_name=name;
    }
    t_thread_name=name;
}


void Thread::join(){
    if(m_thread){
        int rt=pthread_join(m_thread,nullptr);
        if(rt){
             SYLAR_LOG_ERROR(g_logger)<<"pthread_join thread fail,rt="<<rt
                                <<"name="<<m_name;
             throw std::logic_error("pthread_join error");        
        }
        m_thread=0;
    }
}

} 
