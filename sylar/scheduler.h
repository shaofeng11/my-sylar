#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include<memory>
#include"fiber.h"
#include"thread.h"
#include<vector>
#include<list>
#include <iostream>

namespace sylar{

class Scheduler{
public:  
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

     /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * use_caller表示是否复用当前线程.如果复用需要将Scheduler::run()放入协程执行
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads=1,bool use_caller=true,const std::string& name="");
    virtual ~Scheduler();

    const std::string& getName() const {return m_name;}

    //返回当前协程调度器
    static Scheduler* GetThis();

    //返回当前协程调度器的调度协程
    static Fiber* GetMainFiber();

    // 启动协程调度器
    void start();

    //停止协程调度器
    void stop();

    /**
     * @brief 调度协程（加锁）
     * @param[in] fc 协程或函数
     * @param[in] thread 协程执行的线程id,thread=-1不指定运行线程
     */
    template<class FiberOrCb>
    void schedule(FiberOrCb fc,int thread=-1){
        //MutexType::Lock lock(m_mutex);//
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle=scheduleNoLock(fc,thread);
        }
        //若原来，队列中无任务，则添加后，通知调度器有新任务到来
        if(need_tickle){
            tickle();
        }
    }

    //批量添加协程（加锁）
    template<class InputIterator>
    void schedule(InputIterator begin,InputIterator end){
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin!=end){
                //取地址传入，相当去传入指针
                need_tickle=scheduleNoLock(&*begin,-1)||need_tickle;
                begin++;
            }
        }
        if(need_tickle){
            tickle();
        }
    }
protected:
    //通知协程调度器有任务了
    virtual void tickle();
    
    //协程调度函数
    void run();

    //返回是否可以停止
    virtual bool stopping();

    //设置当前的协程调度器
    void setThis();

    //协程无任务可调度时执行idle协程
    virtual void idle();

    bool hasIdThreads() {return m_idleThreadCount>0;}
private:

    //协程调度启动(无锁)
    template<class FiberOrCb>
    //fc:协程或函数  thread 协程执行的线程id,-1标识任意线程
    bool scheduleNoLock(FiberOrCb fc,int thread){
        bool need_tickle=m_fibers.empty();//如果原来的队列为空，那么需要tickle唤醒，返回true
        //有参构造
        FiberAndThread ft(fc,thread);
        //如果ft有协程或函数到来，就将其加入到待执行的协程队列
        if(ft.fiber||ft.cb){
            m_fibers.push_back(ft);
        }
        //若待执行的协程队列为空，返回ture,此时m_fibers已经不为空，需要唤醒任务，从队列中取出协程
        return need_tickle;
    }

private:
        //协程/函数/线程组
    struct FiberAndThread{
        Fiber::ptr fiber;
            // 协程执行函数
        std::function<void()>cb;
        // 指定执行的线程id
        int thread;

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f,int thr)
            :fiber(f),thread(thr){}

            //f:协程指针
        FiberAndThread(Fiber::ptr* f,int thr)
            :thread(thr){
                fiber.swap(*f);
        }
        FiberAndThread(std::function<void()>f,int thr)
            :cb(f),thread(thr){}
        FiberAndThread(std::function<void()>*f,int thr)
            :thread(thr){
            cb.swap(*f);
        }
        //不指定线程
        FiberAndThread()
                :thread(-1){}
        //重置数据
        void reset(){
            fiber=nullptr;
            cb=nullptr;
            thread=-1;
        }
    };
private:
    MutexType m_mutex;

    //线程池数组
    std::vector<Thread::ptr>m_threads;

    // 待执行的协程队列
    std::list<FiberAndThread>m_fibers;

    //std::map<int,std::list<FiberAndThread>>

// use_caller为true时，调度器所在线程的调度协程
    Fiber::ptr m_rootFiber;

    std::string m_name;
protected:
    // 协程下的线程id数组
    std::vector<int>m_threadIds;
    // 线程数量
    size_t m_threadCount=0;

    // 工作线程数量
    std::atomic<size_t> m_activeThreadCount={0};

    // 空闲线程数量
    std::atomic<size_t> m_idleThreadCount={0};

    // 是否正在停止
    bool m_stopping=true;

    // 是否自动停止
    bool m_autoStop=false;

    //主线程id(use_caller的id)
    int m_rootThread=0;
       
};


    
}












#endif