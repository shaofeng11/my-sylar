#include"mutex.h"
#include"macro.h"
namespace sylar{
//构造函数中对信号量进行初始化
Semaphore::Semaphore(uint32_t count){
    //count 指定同时访问的线程数
    if(sem_init(&m_semaphore,0,count)){
        throw std::logic_error("sem_init error");
    }
}
Semaphore:: ~Semaphore(){
    sem_destroy(&m_semaphore);
}

//对信号量进行唤醒，申请
//等待信号量成功返回0，信号量的值减一。
//等待信号量失败返回-1，信号量的值保持不变。
void Semaphore::wait(){
    if(sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
}

//对信号量进行释放
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}

}