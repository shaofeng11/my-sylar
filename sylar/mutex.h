#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include<thread>
#include<memory>
#include<pthread.h>
#include<semaphore.h>
#include<atomic>
#include<functional>
#include <stdint.h>
#include"noncopyable.h"
namespace sylar{
//信号量
class Semaphore :Noncopyable{
public:
     //信号量值的大小
    Semaphore(uint32_t count=0);
    ~Semaphore();


    void wait();//阻塞等待,获取信号量  数量-1
    void notify();//释放信号量+1   数量+1

private:
    //创建一个信号量
    sem_t m_semaphore;
};



//局部锁的模板实现
template<class T>
//Scoped Locking 是将RAII手法应用于locking的并发编程技巧。其具体做法就是在构造时获得锁，在析构时释放锁
struct ScopedLockImpl{
    public:
        ScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.lock();
                m_locked=true;
        }
        ~ScopedLockImpl(){
            unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.lock();
                m_locked=true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;

        // 是否已上锁
        bool m_locked;
};

//局部读锁模板实现
template<class T>
struct ReadScopedLockImpl{
    public:
        ReadScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.rdlock();
                m_locked=true;
            }
        ~ReadScopedLockImpl(){
            unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.rdlock();
                m_locked=true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
};

//局部写锁模板实现
template<class T>
struct WriteScopedLockImpl{
    public:
        WriteScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.wrlock();
                m_locked=true;
            }
        ~WriteScopedLockImpl(){
            unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.wrlock();
                m_locked=true;
            }
        }
        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
};


//普通互斥锁
class Mutex:Noncopyable{
public:
//将T换成Mutex，起别名为Lock
typedef ScopedLockImpl<Mutex> Lock;
    Mutex(){
        pthread_mutex_init(&m_mutex,nullptr);
    }
    ~Mutex(){
         pthread_mutex_destroy(&m_mutex);       
    }

    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;

};




class NullMutex:Noncopyable{
public:
    typedef ScopedLockImpl<NullMutex>Lock;
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}
};


//自旋锁
class SpinLock:Noncopyable{
public:
    typedef ScopedLockImpl<SpinLock> Lock;
    SpinLock(){
        pthread_spin_init(&m_mutex,0);
    }
    ~SpinLock(){
        pthread_spin_destroy(&m_mutex);
    }

     void lock(){
            pthread_spin_lock(&m_mutex);
        }
        void unlock(){
            pthread_spin_unlock(&m_mutex);
        }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock:Noncopyable{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock(){
        m_mutex.clear();
    }
    ~CASLock(){

    }

    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex,std::memory_order_acquire));
    }
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex,std::memory_order_release);
     }
private:
    volatile std::atomic_flag m_mutex;
};


//读写锁
class RWMutex:Noncopyable{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
        RWMutex(){
            pthread_rwlock_init(&m_lock,nullptr);
        }
        ~RWMutex(){
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock(){
            pthread_rwlock_rdlock(&m_lock);//读锁上锁
        }

        void wrlock(){
            pthread_rwlock_wrlock(&m_lock);//写锁上锁
        }

        void unlock(){
            pthread_rwlock_unlock(&m_lock);//解锁
        }
private:
        pthread_rwlock_t m_lock;//读写锁
};

//空读写锁
class NullRWMMutex:Noncopyable{
public:
    typedef ReadScopedLockImpl<NullRWMMutex>ReadLock;
    typedef WriteScopedLockImpl<NullRWMMutex>WriteLock;
    NullRWMMutex(){}
    ~NullRWMMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}
};


}
#endif