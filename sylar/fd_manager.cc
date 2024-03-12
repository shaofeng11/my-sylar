#include"fd_manager.h"
#include<sys/stat.h>
#include"hook.h"
namespace sylar{

FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeOut(-1)
    ,m_sendTimeOut(-1){
    init();
}
FdCtx::~FdCtx(){

}
bool FdCtx::init(){
    if(m_isInit){
        return true;
    }
    m_recvTimeOut=-1;
    m_sendTimeOut=-1;


    struct stat fd_stat;
    if(-1==fstat(m_fd,&fd_stat)){
        m_isSocket=false;
    }else{
        m_isInit=true;
        m_isSocket=S_ISSOCK(fd_stat.st_mode);
    }
    //如果是套接字，设置为系统非阻塞 
    if(m_isSocket){
        int flags=fcntl_f(m_fd,F_GETFL,0);
        //如果句柄不是非阻塞，设置非阻塞 
        if(!(flags&O_NONBLOCK)){
            fcntl_f(m_fd,F_SETFL,flags|O_NONBLOCK);
        }
        m_sysNonblock=true;
    }else{
        m_sysNonblock=false;
    }
    m_userNonblock=false;
    m_isClosed=false;
    return m_isInit;
}
void FdCtx::setTimeout(int type,uint64_t v){
    if(type==SO_RCVTIMEO){//接收超时时间
        m_recvTimeOut=v;
    }else{//发送超时时间
        m_sendTimeOut=v;
    }
}
uint64_t FdCtx::getTimeout(int type){
    if(type==SO_RCVTIMEO){
        return m_recvTimeOut;
    }else{
        return m_sendTimeOut;
    }
}
FdManager::FdManager(){
    m_datas.resize(64);
}
FdCtx::ptr FdManager::get(int fd,bool auto_create){
    if(fd==-1){
        return nullptr;
    }
    RWMutexType::ReadLock lock(m_mutex);
    //如果输入的fd比数组容量大
    if((int)m_datas.size()<=fd){
        //不自动创建
        if(auto_create==false){
            return nullptr;
        }
    //输入的fd在数组中
    }else{
        //如果该fd有数据或不自动创建，返回该fd的上下文(有可能为空)
        if(m_datas[fd]||!auto_create){
            return m_datas[fd];
        }
    }   
    //如果该fd无数据且需自动创建,那么创建一个fdctx添加到数组中
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    m_datas[fd]=ctx;
    return ctx;
}
void FdManager::del(int fd){
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size()<=fd){
        return;
    }
    m_datas[fd].reset();
}
}