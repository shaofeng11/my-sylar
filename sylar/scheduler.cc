#include "scheduler.h"
#include"log.h"
#include"macro.h"
#include"hook.h"
namespace sylar{

sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

//当前线程的调度器，同一个调度器下的所有线程指同一个调度器实例
static thread_local Scheduler* t_scheduler=nullptr;

 //当前线程的调度协程，每个线程都独有一份，包括caller线程
static thread_local Fiber* t_scheduler_fiber=nullptr;


//把调度器所在的线程（称为caller线程）也加入进来作为调度线程
Scheduler::Scheduler(size_t threads,bool use_caller,const std::string& name)
    :m_name(name){
    SYLAR_ASSERT(threads>0);//保证输入线程数无误
    //复用该线程
    if(use_caller){
        sylar::Fiber::GetThis(); 
        //线程数量-1
        --threads;

        SYLAR_ASSERT(GetThis()==nullptr);//保证当前协程没有调度器

        //这一句不明白
        t_scheduler=this;

        //    调度协程初始化，运行run方法，相当于   new Fiber(t_scheduler->run,0,true);
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));
        sylar::Thread::SetName(m_name);

        // 记录当前线程中的执行fiber    
        t_scheduler_fiber=m_rootFiber.get();

        //使用use_caller,则主线程id为当前线程id
        m_rootThread=sylar::GetThreadId();

        m_threadIds.push_back(m_rootThread);
    }else{
        //若不使用usr_caller,则主协程id为-1
        m_rootThread=-1;
    }
    m_threadCount=threads;
}
Scheduler::~Scheduler(){
    SYLAR_ASSERT(m_stopping);
    if(GetThis()==this){
        t_scheduler=nullptr;
    }
}

Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}
Fiber* Scheduler::GetMainFiber(){
    return t_scheduler_fiber;
}


void Scheduler::start(){
    MutexType::Lock lock(m_mutex);
    if(!m_stopping){//false，说明启动了，直接返回
        return;
    }
    //m_stopping为true时,将其改为false
    m_stopping=false;
    SYLAR_ASSERT(m_threads.empty());//保证线程池为空

    m_threads.resize(m_threadCount);

    //创建线程池
    for(size_t i=0;i<m_threadCount;++i){
        // 每个线程绑定Scheduler::run函数
       m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this)
                            ,m_name+"_"+std::to_string(i))); 
        // 记录线程id
       m_threadIds.push_back(m_threads[i]->getId()); 
    }

    //lock.unlock();
    // if(m_rootFiber){
    //     //m_rootFiber->swapIn();
    //     m_rootFiber->call();//切换到执行run那个协程
    //     SYLAR_LOG_INFO(g_logger)<<"call out"<<m_rootFiber->getState();
    // }
}
void Scheduler::stop(){
    m_autoStop=true;
    if(m_rootFiber//只有主线程一个线程且使用user_caller且m_rootFiber状态为INIT或TERM，将m_stopping设为true
                &&m_threadCount==0
                &&(m_rootFiber->getState()==Fiber::TERM
                    ||m_rootFiber->getState()==Fiber::INIT)){
        SYLAR_LOG_INFO(g_logger)<<this<<"stopped";
        m_stopping=true;

        if(stopping()){
            return;
        }
    } 
    if(m_rootThread!=-1){//user_call线程
        SYLAR_ASSERT(GetThis()==this);
    }else{
        SYLAR_ASSERT(GetThis()!=this);
    }

    m_stopping=true;
    // for(size_t i=0;i<m_threadCount;i++){
    //     tickle();
    // }

    // if(m_rootFiber){
    //     tickle();
    // }

    if(m_rootFiber){
        // while(!stopping()){
        //     if(m_rootFiber->getState()==Fiber::TERM
        //             ||m_rootFiber->getState()==Fiber::EXCEPT){
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));
        //         SYLAR_LOG_INFO(g_logger)<<"root fiber is term,reset";
        //         t_scheduler_fiber=m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
        if(!stopping()){
            m_rootFiber->call();
        }
    }
    std::vector<Thread::ptr>thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto& i:thrs){
        i->join();
    }
}
void Scheduler::setThis(){
    t_scheduler=this;
}

void Scheduler::run(){
    SYLAR_LOG_INFO(g_logger)<<m_name<<" run";
    set_hook_enable(true);
    setThis();// t_scheduler=this;
    if(sylar::GetThreadId()!=m_rootThread){
        //在该线程中创建一个调度协程
        t_scheduler_fiber=Fiber::GetThis().get();
    }
     // 空闲协程 后续子类重写Scheduler::idle方法实现epoll_wait阻塞
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this)));
    Fiber::ptr cb_fiber;//回调函数的协程

    // 记录协程、线程、回调函数
    FiberAndThread ft;
    while(true){
        ft.reset();
        bool tickle_me=false;//协程调度器是否有任务
        bool is_active=false;
        {
            MutexType::Lock lock(m_mutex);
            auto it=m_fibers.begin();
            while(it!=m_fibers.end()){
                //如果协程指定了某线程执行，但当前线程不是指定的线程  不处理
                if(it->thread!=-1 && it->thread!=sylar::GetThreadId()){
                    ++it;
                    tickle_me=true;
                    continue;
                }
                //fiber和cb不全为空
                SYLAR_ASSERT(it->fiber||it->cb);

                //fiber不为空，且fiber的状态为EXEC执行态  不处理
                if(it->fiber&&it->fiber->getState()==Fiber::EXEC){
                    ++it;
                    continue;
                }
                ft=*it;
                m_fibers.erase(it);//将该事件从队列中删除
                ++m_activeThreadCount;
                is_active=true;
                break;
            }
            //tickle_me |= it != m_fibers.end();
        }

        //已经取出一个事件
        //协程调度器有任务 唤醒其他线程
        if(tickle_me){ 
            SYLAR_LOG_INFO(g_logger)<<" tickle_me= "<<tickle_me;
            tickle();
        }
        //fiber不为空，且fiber的状态不为TERM结束态或异常态
        // 取出的是协程任务，使用协程执行
        if(ft.fiber&&(ft.fiber->getState()!=Fiber::TERM
                        &&ft.fiber->getState()!=Fiber::EXCEPT)){
            //活跃线程数++
            
            ft.fiber->swapIn();//执行该协程
            //执行完毕，活跃线程数--
            --m_activeThreadCount;
            //执行完后fiber的状态变为READY可执行态即，执行了一半让出了CPU，该协程继续放入管理器
            if(ft.fiber->getState()==Fiber::READY){
                schedule(ft.fiber);
                // 执行完后状态不是TERM和EXCEPT，更改状态为HOLD，下次再次执行
            }else if(ft.fiber->getState()!=Fiber::TERM
                    &&ft.fiber->getState()!=Fiber::EXCEPT){
                ft.fiber->m_state=Fiber::HOLD;
            }   
            ft.reset(); 
            // 取出的是回调函数，放入协程执行 
        }else if(ft.cb){
            if(cb_fiber){
                //若回调不为空，将回调放入协程中
                cb_fiber->reset(ft.cb);
            }else{//若回调为空,将该智能指针reset为一个新的协程，其回调为空
                cb_fiber.reset(new Fiber(ft.cb));
            }
            //将回调放入协程中后，把ft reset清空，然后执行
            ft.reset();
            cb_fiber->swapIn();
            //再回来时，该函数就执行完了，然后查看协程的状态
            --m_activeThreadCount;
            //协程变为READY态，该协程继续放入管理器
            if(cb_fiber->getState()==Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
                // 执行完毕或出错，将协程置空
            }else if(cb_fiber->getState()==Fiber::TERM
                ||cb_fiber->getState()==Fiber::EXCEPT){
                    cb_fiber->reset(nullptr);
                    // 其他状态时（HOLD  EXEC），协程挂起，重置协程，等待下次使用
            }else{ //if(cb_fiber->getState()!=Fiber::TERM){
                cb_fiber->m_state=Fiber::HOLD;
                cb_fiber.reset();
            }
            //没有任务了，ft的fiber和cb都为空，执行idle
        }else{
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            //如果idle的状态为结束状态，跳出循环
            if(idle_fiber->getState()==Fiber::TERM){
                SYLAR_LOG_INFO(g_logger)<<"idle fiber term";
                //tickle();
                break;
            }
            ++m_idleThreadCount;
                // 进入空闲协程 阻塞
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState()!=Fiber::TERM
                    &&idle_fiber->getState()!=Fiber::EXCEPT){
                idle_fiber->m_state=Fiber::HOLD;
            }     
        }
    }
}

void Scheduler::tickle(){
    SYLAR_LOG_INFO(g_logger)<<"tickle";
}
bool Scheduler::stopping(){
    MutexType::Lock lock(m_mutex);
    //若m_stopping=ture，m_fibers为空，活跃线程数也为空，表明可以停止
    return m_autoStop &&m_stopping
            &&m_fibers.empty()&&m_activeThreadCount==0;
}

void Scheduler::idle(){
    SYLAR_LOG_INFO(g_logger)<<"idle";
    //idle时切换当前协程为线程的调度协程，循环执行run,当stopping时退出
    while(!stopping()){
        sylar::Fiber::YieldTohold();
    }
}
}


