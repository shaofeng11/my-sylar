#include "daemon.h"
#include "log.h"
#include"config.h"
#include<string.h>
#include<time.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

namespace sylar{

static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

static sylar::ConfigVar<uint32_t>::ptr g_daemon_restart_restart_interval
=sylar::Config::Lookup("deamon.restart_interval",(uint32_t)5,"daemon restart interval");

std::string ProcessInfo::toString() const{
    std::stringstream ss;
    ss<<"[ProcessInfo parent_id="<<parent_id
      <<" main_id="<<main_id
      <<" parent_start_time="<<sylar::Time2Str(parent_start_time)
      << " main_start_time=" << sylar::Time2Str(main_start_time)
    << " restart_count=" << restart_count << "]";
    return ss.str();    
}

static int real_start(int argc,char** argv,
                    std::function<int(int argc,char** argv)>main_cb){
    ProcessInfoMgr::GetInstance()->main_id = getpid();
    ProcessInfoMgr::GetInstance()->main_start_time = time(0);
    return main_cb(argc,argv);                    
}

static int real_daemon(int argc,char** argv,
                std::function<int(int argc,char** argv)>main_cb){
    daemon(1,0);
    ProcessInfoMgr::GetInstance()->parent_id=getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time=time(0);
    while(true){
        //创建子进程
        pid_t pid=fork();
        //子进程
        if(pid==0){
            ProcessInfoMgr::GetInstance()->main_id=getpid();
            ProcessInfoMgr::GetInstance()->main_start_time=time(0);
            SYLAR_LOG_INFO(g_logger)<<"process start pid="<<getpid();
            return real_start(argc,argv,main_cb);
        }else if(pid<0){
            SYLAR_LOG_ERROR(g_logger)<<"fork error,return="<<pid
                <<" error="<<errno<<" errstr="<<strerror(errno);
            return -1;
        //父进程
        }else{
            int status=0;
            //回收子进程
            waitpid(pid,&status,0);

            //如果回收成功，说明异常出错了
            if(status){
                SYLAR_LOG_ERROR(g_logger)<<"child crash pid="<<pid
                    <<" status"<<status;
            }else{
                //子进程正常结束
                SYLAR_LOG_INFO(g_logger)<<"child finished pid="<<pid;
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count+=1;
            sleep(g_daemon_restart_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc,char** argv,
                std::function<int(int argc,char** argv)>main_cb
                ,bool is_daemon){
    if(!is_daemon){
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
        return real_start(argc,argv,main_cb);
    }
    return real_daemon(argc,argv,main_cb);
}
}