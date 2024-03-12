#include<iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main()
{
// 1、这里定义了一个Logger类的智能指针logger，这里的new会调用Logger的构造函数
// 在构造函数里调用m_formatter.reset函数将输出格式设置为默认值
// m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
// 这里的new会调用LogFormatter的构造函数，会调用LogFormatter::init()方法解析格式
// 根据解析出来的格式项将不同类型的FormatItem添加到m_items里
// 比如说，解析到有%t说明要打印线程号，就将ThreadIdFormatItem添加到m_items里

//     sylar::Logger::ptr logger(new sylar::Logger);//定义一个logger

//     向logger里面加入一个输出地    控制台
//     logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutAppender));//多态？      

//     定义一个logevent，给其初始化为  文件名，行号，0,1,2，事件
//     sylar::LogEvent::ptr event(new sylar::LogEvent(logger,sylar::LogLevel::DEBUG,__FILE__,__LINE__,0,sylar::GetThreadId(),sylar::GetFiberId(),time(0)));

// 4、这里调用logger->log函数，指定打印级别为DEBUG,打印的事件为上一句创建的event
// logger自身有一个成员m_level，也是一个日志打印级别，表示这个logger只打印m_level级别及以上的日志
// 在logger->log函数里，会对比m_level和指定的打印级别(这里是DEBUG)，然后遍历logger里的所有appender，调用每个appender的log函数(这里只有一个StdoutLogAppender)
// appender里的log函数会调用m_formatter->format函数打印到不同的appender上。
// m_formatter->format函数里会遍历m_items里的FormatItem，调用所有类别的FormatItem的format函数，比如是线程号的FormatItem，就将线程号写入logger的stringstream流里

//     logger->log(sylar::LogLevel::DEBUG,event);
//     std::cout<<"hello sylar log"<<std::endl;




//1、同上
sylar::Logger::ptr logger(new sylar::Logger); 
//2、添加StdoutLogAppender和FileLogAppender，设置了FileLogAppender的打印格式和打印级别
logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutAppender)); 
sylar::FileAppender::ptr file_appender(new sylar::FileAppender("./log.txt"));
sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
file_appender->setFormatter(fmt);
file_appender->setLevel(sylar::LogLevel::ERROR);
logger->addAppender(file_appender);
//3、使用这两个宏通过流形式将不同的级别的日志写入到logger中
//这两个宏都是调用的是SYLAR_LOG_LEVEL(logger,level)这个宏，只是level设置不一样
//在SYLAR_LOG_LEVEL里，如果logger->getLevel()<=level，会创建一个LogEventWrap对象
//LogEventWrap对象只是将LogEvent封装了一下，里面还是创建了一个LogEvent对象的智能指针，最后返回LogEvent对象的流对象
//因此最后SYLAR_LOG_INFO(logger) 最后返回的是stringstream流对象，可以用<<将字符串写入
//最后在LogEventWrap对象的析构函数中，调用了logger的log函数，遍历logger里的所有appender，调用appender的log函数
//SYLAR_LOG_INFO(logger) << "test macro";
//SYLAR_LOG_ERROR(logger) << "test macro error";
SYLAR_LOG_FMT_ERROR(logger,"test macro fmt error %s","aad");

auto l=sylar::LoggerMgr::GetInstance()->getLogger("xx");
SYLAR_LOG_INFO(l)<<"xxx";
  return 0;
}