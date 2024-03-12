#include"timer.h"
#include"util.h"
namespace sylar{

bool Timer::Comparator::operator()(const Timer::ptr& lhs,const Timer::ptr& rhs) const{
    if(!lhs&&!rhs){
        return false;
    }
    if(!lhs){
        return true;
    }
    if(!rhs){
        return false;
    }
    if(lhs->m_next < rhs->m_next){
        return true;
    }
    if(lhs->m_next > rhs->m_next){
        return false;
    }
    //事件都一样，比地址
    return lhs.get()<rhs.get();
}
Timer::Timer(uint64_t ms,std::function<void()>cb,
    bool recurring,TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager){
        m_next=sylar::GetCurrentMs()+m_ms;
}

 Timer::Timer(uint64_t next)
 :m_next(next){
 }

bool Timer::cancel(){
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    //先把定时器的回调置空，然后在m_manager中的m_timers中找到自己并删除
    if(m_cb){
        m_cb=nullptr;
        auto it=m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}
bool Timer::refresh(){
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    //如果回调为空,说明已被取消
    if(!m_cb){
        return false;
    }
    //找到定时器
    auto it=m_manager->m_timers.find(shared_from_this());
    if(it==m_manager->m_timers.end()){
        return false;
    }
    //先删除。再重新插入
    m_manager->m_timers.erase(it);
    m_next=sylar::GetCurrentMs()+m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms,bool from_now){
    //如果时间没变就不用改了
    if(ms==m_ms&&!from_now){
        return false;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb){
        return false;
    }
    auto it=m_manager->m_timers.find(shared_from_this());
    if(it==m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);

    uint64_t start=0;
    if(from_now){
        start=sylar::GetCurrentMs();
    }else{ 
        start=m_next-m_ms;//??
    }
    m_ms=ms;
    m_next=start+m_ms;
    m_manager->addTimer(shared_from_this(),lock);
    return true;
}
TimerManager::TimerManager(){
    m_previousTime=sylar::GetCurrentMs();
}
TimerManager::~TimerManager(){

}

Timer::ptr TimerManager::addTimer(uint64_t ms,std::function<void()>cb
                        ,bool recurring){
    //创建定时器
    Timer::ptr timer(new Timer(ms,cb,recurring,this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer,lock);
    return timer;
}
//weak_ptr不会使引用计数加1，但可以知道指针是否被释放
//lock返回其所指对象的shared_ptr智能指针(对象销毁时返回”空”shared_ptr)
static void onTimer(std::weak_ptr<void> weak_cond,std::function<void()>cb){
    std::shared_ptr<void>tmp=weak_cond.lock();
    if(tmp){
        cb();
    }
}
Timer::ptr TimerManager::addConditionTimer(uint64_t ms,std::function<void()>cb
                        ,std::weak_ptr<void>weak_cond,bool recurring){
    //onTimer作为回调函数 
    return addTimer(ms,std::bind(&onTimer,weak_cond,cb),recurring);
}

 uint64_t TimerManager::getNextTime(){
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled=false;
    if(m_timers.empty()){
        return ~0ull;//无穷大
    }
    //取m_timers的第一个
    const Timer::ptr& next=*m_timers.begin();
    uint64_t now_ms=sylar::GetCurrentMs();
    if(now_ms>=next->m_next){
        return 0;//如果当前时间大于第一个定时器的时间，返回0，马上执行
    }else{
        return next->m_next-now_ms;//返回剩余时间
    }
 }

//获取需要执行的定时器的回调函数列表
  void TimerManager::listExpiredCb(std::vector<std::function<void()>>&cbs){
        uint64_t now_ms=sylar::GetCurrentMs();
        std::vector<Timer::ptr>expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty()){
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
        //服务器时间是否被调后了
        bool rollover=detectClockRollover(now_ms);
        //没有调后且下一个timer的执行时间大于当前时间（没有超时），return
        if(!rollover&&((*m_timers.begin())->m_next>now_ms)){
            return;
        }   

        Timer::ptr now_timer(new Timer(now_ms));
        ///若rollover=true，将m_timers全部清理
        auto it=rollover? m_timers.end():m_timers.lower_bound(now_timer);//指向now_timer的下一个迭代器
        while(it!=m_timers.end()&&(*it)->m_next==now_ms){
            ++it;
        }
        expired.insert(expired.begin(),m_timers.begin(),it);//m_timers.begin()到it超时了，都要执行
        m_timers.erase(m_timers.begin(),it);
        //分配长度
        cbs.reserve(expired.size());

        for(auto& timer:expired){
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring){
                timer->m_next=now_ms+timer->m_ms;
                m_timers.insert(timer);
            }else{
                timer->m_cb=nullptr;
            }
        }
  }
  void TimerManager::addTimer(Timer::ptr val,RWMutex::WriteLock& lock){
    //插入到m_timers，it是迭代器的位置
    auto it=m_timers.insert(val).first;
    //判断是否在m_timers的首部并且m_tickled=false，在首部说明是一个即将执行的定时器
    bool at_front=(it==m_timers.begin())&&!m_tickled;
    lock.unlock();
    if(at_front){
        m_tickled=true;       
    }

    if(at_front){
        //唤醒epoll_wait,重新设置定时时间
        onTimerInsertedAtFront();
    }
  }
bool TimerManager::detectClockRollover(uint64_t now_ms){
    bool rollover=false;
    //如果当前事件比上次定时器触发时间小一个小时，说明往前调了
    if(now_ms<m_previousTime&&
        now_ms<(m_previousTime-60*60*1000)){
            rollover=true;
        }
    m_previousTime=now_ms;
    return rollover;
}

bool TimerManager::hasTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}
}