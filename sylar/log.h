#pragma once
#ifndef _SYLAR_LOG_H__
#define _SYLAR_LOG_H__
#include<string.h>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>//??
#include<fstream>
#include <stdarg.h>
#include<vector>
#include "util.h"
#include "singleton.h"
#include<map>
#include"thread.h"

//流式的宏
//使用流式方式将日志级别level的日志写入到logger

	//logger是日志器名称，level是级别
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0),sylar::Thread::GetName()))).getSS()
  //一句话创建了2个对象，1.LogEventWrap,用LogEvent初始化
  //2.LogEvent，用LogEvent(logger,level,__FILE__,__LINE__,0,sylar::GetThreadId(),
                //sylar::GetFiberId(),time(0)))).getSS()初始化
//创建对象后，调用wrap.getSS()方法，获取日志流对象。

//使用流式方式将日志级别debug的日志写入到logger
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::DEBUG)
//使用流式方式将日志级别info的日志写入到logger
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::INFO)

#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::WARN)

#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::ERROR)

#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::FATAL)

//用格式化的方式将日志级别Level的日志写入logger
//...表示变参
#define SYLAR_LOG_FMT_LEVEL(logger,level,fmt, ...) \
if(logger->getLevel()<=level) \
	sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger,level,\
	__FILE__,__LINE__,0,sylar::GetThreadId(),\
	sylar::GetFiberId(),time(0),sylar::Thread::GetName()))).getEvent()->format(fmt,__VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::INFO, fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::WARN, fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::ERROR, fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::FATAL, fmt,__VA_ARGS__)


//root的宏
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

//得到日志器的名字
#define SYLAR_LOG_NAME(name)   sylar::LoggerMgr::GetInstance()->getLogger(name);

namespace sylar//区分和别人代码重命名的部分
{
	class Logger;

	class Loggermanager;

		//日志级别
	class LogLevel
	{
	public:
		enum Level
		{
			UNKNOW=0,
			DEBUG = 1,//将DEBUG赋值为1
			INFO,
			WARN,
			ERROR,
			FATAL
		};
		static const char* Tostring(LogLevel::Level level);
		static LogLevel::Level FromString(const std::string& str);
	};

	//日志事件

	//LogEvent是存放信息的地方，包括行、列、线程号、协程号、内容等
	class LogEvent {         //将每个日志定义成logEvent,相当于格式化？
	public:
		typedef std::shared_ptr<LogEvent>ptr;//智能指针
		const char* getFile() const { return m_file; }
		int32_t getLine()const { return m_line; }
		uint32_t getElapse()const{ return m_elapse; }
		uint32_t getThreadId()const { return m_threadId; }
		uint32_t getFiberId()const { return m_fiberId; }
		uint64_t getTime()const { return m_time; }
		const std::string& getThreadName() const { return m_threadName;}

		//返回日志内容
		std::string getContent() const { return m_ss.str(); }
		std::shared_ptr<Logger> getLogger() const { return m_logger;}
		LogLevel::Level getLevel() const { return m_level;}
		std::stringstream& getSS() { return m_ss; }
		LogEvent(std::shared_ptr<Logger>logger,LogLevel::Level level, const char* file, int32_t line, uint32_t elapse
			, uint32_t thread_id, uint32_t fiber_id, uint64_t time
			,const std::string& thread_name);//logEvent构造函数

//logEvent中新增的两个format方法主要是为了让我们能用格式字符串的方式来输入日志信息：
//即允许在日志内容中出现%x来进行格式化

//使用省略号（即三个点）来表示函数可以处理的参数数量不定
		void format(const char* fmt, ...);

		void format(const char* fmt, va_list al);
	private:
		const char* m_file=nullptr;//文件名
		int32_t m_line=0 ;          //行号//确定大小的整型
		uint32_t m_elapse=0 ;       //程序启动到现在的毫秒数
		uint32_t m_threadId=0 ;     //线程id
		uint32_t m_fiberId=0 ;      //协程id
		uint64_t m_time=0;             //时间戳
		std::string m_threadName;    //线程名称
		std::stringstream m_ss;       //消息内容
		std::shared_ptr<Logger> m_logger;   //日志器
		LogLevel::Level m_level;       //日志级别
	};

class LogEventWrap{
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();

//获取日志内容流
	std::stringstream& getSS();

//返回该事件本身
	LogEvent::ptr getEvent() const { return m_event;}
private:
LogEvent::ptr m_event;
};

class LogFormatter
{
public:
	typedef std::shared_ptr<LogFormatter> ptr;

	//构造函数，创建时会传入pattern将其传给m_pattern;
	LogFormatter(const std::string& pattern);
	//%t %thread_id %m%n

	//将LogEvent格式化成字符串,返回格式化日志文本
	std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);//传入event，转化为string提供给appender输出

public:
	/*%m 消息
		*  %p 日志级别
		*  %r 累计毫秒数
		*  %c 日志名称
		*  %t 线程id
		*  %n 换行
		*  %d 时间
		*  %f 文件名
		*  %l 行号
		*  %T 制表符
		*  %F 协程id
		*  %N 线程名称*/
//将日志内容项格式化  


class FormatItem //虚类      
{
public:
	typedef std::shared_ptr<FormatItem> ptr;
	FormatItem(const std::string& fmt = "") {};
	virtual ~FormatItem(){}  //虚析构函数


	//传入event，输出到流文件os里面
	//为了得到日志级别，将Level作为参数传进来；为了得到日志器名称，将日志器logger作为参数传进来

	//format函数将event输出为string
	virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event)=0;
};
void init();//日志格式pattern的解析

//返回日志模版
const std::string getPattern() const{return m_pattern;}

bool isError() const {return m_error;}

private:
	//日志格式模板
	//日志格式举例：%d{ %Y - %m - %d %H:%M : %S }%T%t%T%N%T%F%T[%p] % T[%c] % T%f : %l%T%m%n
	std::string m_pattern;  //根据m_pattern的格式来解析item的信息

		//日志格式解析后格式       输出多少个项，里面存放的是具体项的智能指针
	std::vector<FormatItem::ptr>m_items; 

	//formatter是否有错误
	bool m_error=false;
};


//日志输出地
//LogAppender是输出地，可以选择文件或控制台进行输出，里面也包含了LogFormatter对象，来保证不同的输出地可以以不同的格式呈现
class LogAppender
{
friend class Logger;	
public:
	typedef std::shared_ptr<LogAppender>ptr;
	typedef Mutex MutexType;
	virtual ~LogAppender() {}    //虚析构函数
	void setFormatter(LogFormatter::ptr val);//输出地想以什么格式输出？
	LogFormatter::ptr getFormatter();//提供接口，得到m_formatter
	void setLevel(LogLevel::Level val) {m_level=val;}
	LogLevel::Level getLevel() const { return m_level;}
	virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)=0;//写入日志，纯虚函数,子类必须重写

	//将日志输出目标的配置转成YAML String
	virtual	std::string toYamlString()=0;
protected://父类的属性设置成protceted，子类才能使用

//默认的日志级别：DEBUG，可以在子类中更改
	LogLevel::Level m_level=LogLevel::DEBUG;
	bool hasFormatter =false;
	MutexType m_mutex;
	LogFormatter::ptr m_formatter;//输出的格式

};

//日志输出器
//Logger是日志器，作为控制中枢可以将同一个信息添加多个LogAppender和LogFormatter对象来保证可以同时往多个地方输出
class Logger:public::std::enable_shared_from_this<Logger>
{
	//友元
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr;
	typedef Mutex MutexType;
	Logger(const std::string& name="root");//如果不传参，则默认root

	//写入日志，指定日志级别
	void log(LogLevel::Level level, LogEvent::ptr event);//写日志
	void debug(LogEvent::ptr event);//写debug日志
	void info(LogEvent::ptr event);//写info日志
	void warn(LogEvent::ptr event);//写warn日志
	void error(LogEvent::ptr event);//写error日志
	void fatal(LogEvent::ptr event);//写fatal日志
	void addAppender(LogAppender::ptr appender);//添加appender  
	void delAppender(LogAppender::ptr appender);
	void clearAppenders();  //清空目标日志
	LogLevel::Level getLevel() const { return m_level; }//什么写法？    获取日志级别  		//因为是private，所以要提供函数接口供外部使用
	void setLevel(LogLevel::Level val) { m_level = val; }
	
	//设置日志格式器
		void setFormatter(LogFormatter::ptr val);

	//设置日志格式模板
		void setFormatter(const std::string& val);


	//获取日志格式器
	LogFormatter::ptr getFormatter();

	const std::string& getName() const { return m_name; }

	//将日志输出目标的配置转成YAML String
	std::string toYamlString();
private:
	std::string m_name;                     //日志名称

	//因为是private，所以要提供函数接口供外部使用
	LogLevel::Level m_level;           //日志级别    //传入日志级别  满足日志级别的才输出 
	sylar::Mutex m_mutex;
	std::list<LogAppender::ptr> m_appenders;//Appender     输出到目的地集合
	LogFormatter::ptr m_formatter; //日志格式器

	//主日志器
	Logger::ptr m_root;
};

//日志格式器
	//LogFormatter是安排数据摆放顺序，包含模式解析等功能（可以看看log4j标准)


//输出到控制台的Appender
class StdoutAppender :public LogAppender
{
public:
	typedef std::shared_ptr<StdoutAppender> ptr;
	virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override;

	std::string toYamlString() override;
};
//输出到文件的Appender
class FileAppender :public LogAppender
{
public:
	typedef std::shared_ptr<FileAppender> ptr;
	FileAppender(const std::string& filename);//构造函数
	//重新打开文件，文件打开成功则为true
	bool reopen();
	virtual void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;

	std::string toYamlString() override;
private:
	std::string m_filename;//输出的文件名
	
	std::ofstream m_filestream;//输出流文件

/// 上次重新打开时间
	uint64_t m_lastTime = 0;
};



class LoggerManager{
public:
	typedef Mutex MutexType;
	LoggerManager();//设置缺省logger：m_root中包含仅一个输出地：stdLogAppender
	Logger::ptr getLogger(const std::string& name);//调用find按名字找对应的logger，没找到则返回默认的日志器m_root

	void init();//之后会从配置文件中直接加载我们要使用的logger

	Logger::ptr getRoot() const {return m_root;}

	//将日志输出目标的配置转成YAML String
	std::string toYamlString();
private:
	MutexType m_mutex;
	//日志器容器
	std::map<std::string,Logger::ptr>m_loggers;

	// 主日志器
	Logger::ptr m_root;

};

// 日志器管理类单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;
}

#endif