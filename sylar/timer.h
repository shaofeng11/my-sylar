#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__
#include<memory>
#include"thread.h"
#include"mutex.h"
#include<set>
#include<vector>
namespace sylar{

class TimerManager;
    //enable_shared_from_this该类只能通过智能指针使用
    //成员函数中可以用shared from this获得自身的智能指针
class Timer:public std::enable_shared_from_this<Timer>{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer>ptr;

    bool cancel();
    bool refresh();

     /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms,bool from_now);
private:
//recurring循环定时器
    Timer(uint64_t ms,std::function<void()>cb,
        bool recurring,TimerManager* manager);
    Timer(uint64_t next);

private:
    bool m_recurring=false;     //是否循环定时器
    uint64_t m_ms=0;            //执行周期
    uint64_t m_next=0;          //精确的执行时间
    std::function<void()>m_cb;
    //定时器管理器
    TimerManager* m_manager=nullptr;
private:
    struct  Comparator
    {
        //比较定时器的智能指针的大小(按执行时间排序)
        bool operator()(const Timer::ptr& lhs,const Timer::ptr& rhs) const;
    };   
};


class TimerManager{
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();
    Timer::ptr addTimer(uint64_t ms,std::function<void()>cb
                        ,bool recurring=false);
    Timer::ptr addConditionTimer(uint64_t ms,std::function<void()>cb
                        ,std::weak_ptr<void>weak_cond,bool recurring=false);//如果智能指针不存在了，就不用执行了
     //到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTime();

    //获取需要执行的定时器的回调 函数列表
    void listExpiredCb(std::vector<std::function<void()>>&cbs);
protected:
//当有新的定时器插入到定时器的首部,执行该函数
    virtual void onTimerInsertedAtFront()=0;
    //将定时器添加到管理器中
    void addTimer(Timer::ptr val,RWMutex::WriteLock& lock);

    bool hasTimer();
private:
    //检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    std::set<Timer::ptr,Timer::Comparator>m_timers;
    // 是否触发onTimerInsertedAtFront
    bool m_tickled=false;
    // 上次执行时间
    uint64_t m_previousTime=0;
};
}

#endif