#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include<thread>
#include<functional>
#include<memory>
#include<pthread.h>
#include<semaphore.h>
#include<atomic>
#include"mutex.h"

namespace sylar{

//互斥量和互斥信号量都是不能拷贝的，拷贝会破坏他们的作用
class Thread{
public:
    typedef std::shared_ptr<Thread>ptr;
    //构造函数，创建线程时，要给它一个名字和回调
    Thread(std::function<void()>cb,const std::string& name);
    //析构
    ~Thread();

    pid_t getId() const {return m_id;}
    const std::string& getName() const {return m_name;}


    void join();

    //获取当前的线程指针,拿到当前线程
    static Thread* GetThis();
    static const std::string& GetName();//返回线程名给日志用，用static，静态成员方法
    static void SetName(const std::string& name); //设置线程名
private:
//禁用构造函数，禁用=号重载
    Thread(const Thread&)=delete;
    Thread(const Thread&&)=delete;
    Thread(& operator=(const Thread&))=delete;

    //线程回调函数
    static void* run(void* arg);

private:
//进程id   线程id  线程回调函数(用来做什么事)  线程名   信号量   
    pid_t m_id=-1;//进程id
    pthread_t m_thread=0;
    std::function<void()>m_cb;
    std::string m_name;//线程名

    Semaphore m_semaphore;
};

}
#endif