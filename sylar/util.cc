#include "util.h"
#include<execinfo.h>
#include<sys/time.h>
#include"log.h"
#include "fiber.h"
namespace sylar{

static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

    pid_t GetThreadId(){
        return syscall(SYS_gettid);
    }
    uint32_t GetFiberId(){
        return sylar::Fiber::GetFiberId();
    }

void Backtrace(std::vector<std::string>& bt,int size,int skip){
    //malloc函数返回一个指向新分配内存块的指针，在堆区
    void** array=(void**)malloc((sizeof(void*)*size));

    //array为传出参数。。获取当前线程的调用堆栈,获取的信息将会被存放在buffer中,它是一个指针列表。
    size_t s=::backtrace(array,size);

    //将array中的内容转换成字符串
    //函数返回值是一个指向字符串数组的指针,它的大小同buffer相同.
    //每个字符串包含了一个相对于buffer中对应元素的可打印信息.它包括函数名，函数的偏移地址,和实际的返回地址。
    char** strings=backtrace_symbols(array,s);
    if(strings==NULL){
        SYLAR_LOG_ERROR(g_logger)<<"backtrace_symbols error";
        return;
    }
    for(size_t i=skip;i<s;++i){
        //将信息放到bt容器中
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}
std::string BacktraceToString(int size,int skip,const std::string& prefix){
    std::vector<std::string>bt;
    Backtrace(bt,size,skip);
    std::stringstream ss;
    for(size_t i=0;i<bt.size();++i){
        ss<<prefix<<bt[i]<<std::endl;
    }
    return ss.str();
}

//getimeofday获取到的是自1970-01-01 00:00:00 +0000 (UTC)以来的秒数和微秒数。
uint64_t GetCurrentMs(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000 + tv.tv_usec / 1000;
}
uint64_t GetCurrentUs(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000*1000ul +tv.tv_usec;
}

std::string Time2Str(time_t ts,const std::string& format){
    struct tm tm;
    localtime_r(&ts,&tm);
    char buf[64];
    strftime(buf,sizeof(buf),format.c_str(),&tm);
    return buf;
}

time_t Str2Time(const char *str, const char *format){
    struct tm t;
    memset(&t,0,sizeof(t));
    if(!strptime(str,format,&t)){
        return 0;
    }
    return mktime(&t);
}



}