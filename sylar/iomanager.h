#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__
#include"scheduler.h"
#include"timer.h"
namespace sylar{

class IOManager:public Scheduler,public TimerManager{
public:
    typedef std::shared_ptr<IOManager>ptr;
    typedef RWMutex RWMutexType;
    enum Event{
        NONE=0x0,
        READ=0x1,
        WRITE=0x4
    };
private:
    //Socket事件上线文类
    //每一个fd对应一个fdcontext
    struct FdContext{
        typedef Mutex MutexType;
        /**
     * @brief 事件上下文类
     * @details fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调函数的调度器
     *          sylar对fd事件做了简化，只预留了读事件和写事件，所有的事件都被归类到这两类事件中
     */
        struct EventContext{
            Scheduler* scheduler=nullptr;//待执行的schduler
            Fiber::ptr fiber;            //事件协程
            std::function<void()>cb;     //事件回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
         /**
         * @brief 触发事件
         * @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        int fd;              //事件关联的句柄
        EventContext read;   //读事件
        EventContext write;  //写事件
        //该fd添加了哪些事件的回调函数，或者说该fd关心哪些事件
        Event events=NONE; //已经注册的事件
        MutexType mutex;     
    };
public:
    IOManager(size_t threads=1,bool use_caller=true,const std::string& name="");
    ~IOManager();

    //0 success  -1error
    int addEvent(int fd,Event event,std::function<void()>cb=nullptr);

    bool delEvent(int fd,Event event);

    bool cancelEvent(int fd,Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();
protected:
    void tickle() override;

    bool stopping() override;

// O协程调度器在idle时会epoll_wait所有注册的fd，
// 如果有fd满足条件，epoll_wait返回，从私有数据中拿到fd的上下文信息，并且执行其中的回调函数。
// （实际是idle协程只负责收集所有已触发的fd的回调函数并将其加入调度器的任务队列，
// 真正的执行时机是idle协程退出后，调度器在下一轮调度时执行）
    void idle() override;

    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);

    bool stopping(uint64_t& timeout);

private:
    int m_epfd=0;

    //pipe文件句柄
    int m_tickleFds[2];

    //当前等待执行的事件数量
    std::atomic<size_t>m_pendingEventCount={0};
    RWMutexType m_mutex;

    // socket事件上下文的容器
    std::vector<FdContext*>m_fdContexts;
};

}


#endif