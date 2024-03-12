#include<iostream>
#include<tuple>
#include<map>
#include<functional>
#include<time.h>
#include<string.h>
#include "util.h"
#include <pthread.h>
#include<set>
#include"log.h"
#include"config.h"

namespace sylar {


const char* LogLevel::Tostring(LogLevel::Level level)
{
	switch (level){
	case sylar::LogLevel::DEBUG:
		return "DEBUG";
		break;
	case sylar::LogLevel::INFO:
		return "INFO";
		break;
	case sylar::LogLevel::WARN:
		return "WARN";
		break;
	case sylar::LogLevel::ERROR:
		return "ERROR";
		break;
	case sylar::LogLevel::FATAL:
		return "FATAL";
		break;
	default:
		return "UNKONW";
		break;
	}
	return "UNKNOW";
}

	LogLevel::Level LogLevel::FromString(const std::string& str){
	#define XX(level,v) \
	if(str==#v){ \
		return LogLevel::level; \
	} 
	XX(DEBUG,debug);
	XX(INFO,info);
	XX(WARN,warn);
	XX(ERROR,error);
	XX(FATAL,fatal);

	XX(DEBUG,DEBUG);
	XX(INFO,INFO);
	XX(WARN,WARN);
	XX(ERROR,ERROR);
	XX(FATAL,FATAL);
	return LogLevel::UNKNOW;
	#undef XX
}


//这里用wrap的原因是，wrap作为临时对象，在使用完后直接析构，触发日志写入，
//然而日志本身的智能指针，如果声明在主函数里面，程序不结束就永远无法释放

//使用临时对象来打印日志
LogEventWrap::LogEventWrap(LogEvent::ptr e)
	:m_event(e){
}
LogEventWrap::~LogEventWrap(){
	//《wrap在析构的时候把event写到log里》
	m_event->getLogger()->log(m_event->getLevel(),m_event);

}
std::stringstream& LogEventWrap::getSS(){
	return m_event->getSS();
}


void LogAppender::setFormatter(LogFormatter::ptr val){
	//锁在执行完这个函数后自己释放，释放的时候执行析构函数自动销毁锁
	MutexType::Lock lock(m_mutex);
	m_formatter=val;
	if(m_formatter){
		hasFormatter=true;
	}else{
		hasFormatter=false;
	}
}
LogFormatter::ptr LogAppender::getFormatter(){
	MutexType::Lock lock(m_mutex);
	return m_formatter;
}

	//日志信息
	class MessageFormatItem : public LogFormatter::FormatItem
	{
	public:
		MessageFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->getContent();
		}
	};

//日志级别
class LevelFormatItem : public LogFormatter::FormatItem
{
public:
	LevelFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << LogLevel::Tostring(level);
	}
};

//日志启动消耗时间
class elapseFormatItem : public LogFormatter::FormatItem
{
public:
	elapseFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << event->getElapse();
	}
};

//日志器名称
class nameFormatItem : public LogFormatter::FormatItem
{
public:
	nameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << event->getLogger()->getName();
	}
};

//线程号
class threadIdFormatItem : public LogFormatter::FormatItem
{
public:
	threadIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << event->getThreadId();
	}
};


//日志时间.
class dataTimeFormatItem : public LogFormatter::FormatItem
{
public:
	dataTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%s") :m_format(format) {
		if(m_format.empty())
		{
			m_format="%Y-%m-%d %H:%M:%s";
		}
	}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		//本地时间结构体
		struct tm tm;
		time_t time=event->getTime();

		//返回一个指向结构体tm的指针
		localtime_r(&time,&tm);
		char buf[64];
		strftime(buf,sizeof(buf),m_format.c_str(),&tm);
		os << buf;
	}
private:
	std::string m_format;//时间输出格式   要以字符串的形式输出
};
//文件名
class fileNameFormatItem : public LogFormatter::FormatItem
{
public:
	fileNameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << event->getFile();
	}
};
//行号
class lineFormatItem : public LogFormatter::FormatItem
{
public:
	lineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << event->getLine();
	}
};


//stringFormatItem的作用？
class stringFormatItem : public LogFormatter::FormatItem
{
public:
	stringFormatItem(const std::string& str) :FormatItem(str), m_string(str) {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << m_string;

	}
private:
	std::string m_string;
};

//换行符
class newlineFormatItem : public LogFormatter::FormatItem
{
public:
	newlineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << std::endl;
	}
};


	//协程Id
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};
//线程名
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};
	//Tab
class TabFormatItem : public LogFormatter::FormatItem
{
public:
	TabFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
	{
		os << "\t";
	}
};

LogEvent::LogEvent(std::shared_ptr<Logger>logger,LogLevel::Level level
				, const char* file, int32_t line, uint32_t elapse
				, uint32_t thread_id, uint32_t fiber_id, uint64_t time
				,const std::string& thread_name)
		:m_file(file),
		m_line(line),
		m_elapse(elapse),
		m_threadId(thread_id),
		m_fiberId(fiber_id),
		m_time(time),
		m_threadName(thread_name),
		m_logger(logger),
		m_level(level){
}

void LogEvent::format(const char* fmt, ...){
	//定义一个 `va_list` 变量 `al`
	va_list al;
	 //使 al 指向 fmt 后面那个参数，栈中参数地址连续，所以可以做到
	va_start(al,fmt);
	//函数重载，调用下面那个
	format(fmt,al);
	va_end(al);
}

void LogEvent::format(const char* fmt, va_list al){
	char* buf=nullptr;
	int len=vasprintf(&buf,fmt,al);
	if(len!=-1){
		m_ss<<std::string(buf,len);
		free(buf);
	}
}

Logger::Logger(const std::string& name) 
:m_name(name)
,m_level(LogLevel::DEBUG){
	//这一句给formater的m_partern进行初始化赋值，即为%d [%Y-%m-%d %H:%M%S]%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
	//调用init()函数，对partern进行解析，
	//解析过后，放入vec容器中，然后看解析的项是否在map容器s_format_items中，若存在，则在m_items中添加相应的项

	//2022-11-09 16:15:18	30030	0	[INFO]	[root]	/code/sylar/workspace/sylar/tests/test.cc:21	test macro
//     %d时间	     线程号  协程号 [日志级别] [日志器名称]      文件名:行号	         日志内容即消息体  换行										
//中间穿插着tab即%T

//log的formatter，按照log的格式进行解析
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));//智能指针的初始化，使其指向"",
}
void Logger::addAppender(LogAppender::ptr appender)//添加appender
{
	MutexType::Lock lock(m_mutex);
	//如果appender没有formatter，那appender就用log的appender
	if (!appender->getFormatter()){
		MutexType::Lock ll(appender->m_mutex);
		appender->m_formatter=m_formatter;//appender的formatter赋值为logger的formatter
	}
	m_appenders.push_back(appender);//添加一个日志输出器

}
void Logger::delAppender(LogAppender::ptr appender)
{
	MutexType::Lock lock(m_mutex);
	for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
	{
		if (*it == appender)
		{
			m_appenders.erase(it); //删除一个日志输出器
			break;
		}
	}
}
void Logger::clearAppenders(){  //清空目标日志
	MutexType::Lock lock(m_mutex);
	m_appenders.clear();
}

//设置日志格式器
void Logger::setFormatter(LogFormatter::ptr val){
	MutexType::Lock lock(m_mutex);
	m_formatter=val;

	for(auto& i:m_appenders){
		MutexType::Lock ll(i->m_mutex);
		if(!i->hasFormatter){
			i->m_formatter=m_formatter;
		}
	}
}

//设置日志格式模板
void Logger::setFormatter(const std::string& val){
	sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
	if(new_val->isError()){
		std::cout<<"Logger setFormatter name="<<m_name
				<<"value="<<val<<"invalid formatter"
				<<std::endl;
		return;
	}
	//m_formatter=new_val;
	setFormatter(new_val);
}

LogFormatter::ptr Logger::getFormatter(){
	MutexType::Lock lock(m_mutex);
	return m_formatter;
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
	if(level >= m_level){//遍历输出到每个appender里面
		auto self = shared_from_this();// Logger::ptr  self  为什么不行
		MutexType::Lock lock(m_mutex);
		if(!m_appenders.empty()){
			for (auto& i : m_appenders)//遍历m_appenders里的元素
			{
				i->log(self,level, event);//appender里的log函数，用来输出日志
			}
		}else if(m_root){
			m_root->log(level,event);
		}
	}
}
void  Logger::debug(LogEvent::ptr event)
{
	log(LogLevel::DEBUG,event);
}
void  Logger::info(LogEvent::ptr event)
{
	log(LogLevel::INFO,event);
}
void  Logger::warn(LogEvent::ptr event)
{
	log( LogLevel::WARN,event );
}
void  Logger::error(LogEvent::ptr event)
{
	log( LogLevel::ERROR,event );
}
void  Logger::fatal(LogEvent::ptr event)
{
	log( LogLevel::FATAL,event);
}
std::string Logger::toYamlString(){
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["name"]=m_name;
	if(m_level!=LogLevel::UNKNOW)
		node["level"]=LogLevel::Tostring(m_level);
	if(m_formatter)
		node["formatter"]=m_formatter->getPattern();
	
	for(auto& i:m_appenders){
		node["appenders"].push_back(YAML::Load(i->toYamlString()));
	}
	std::stringstream ss;
	ss<<node;
	return ss.str();
}
FileAppender::FileAppender(const std::string& filename)
	:m_filename(filename){
	reopen();
}
bool FileAppender::reopen(){
	MutexType::Lock lock(m_mutex);
	if (m_filestream){
		m_filestream.close();
	}
	m_filestream.open(m_filename);
	return !!m_filestream;//!!将非0值转化为1，0还是0
}
void FileAppender::log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event){
	if (level >= m_level){
		uint64_t now=time(0);
		if(now!=m_lastTime){
			reopen();
			m_lastTime=now;
		}
		MutexType::Lock lock(m_mutex);
		m_filestream<< m_formatter->format(logger,level,event);
	}
}


std::string FileAppender::toYamlString(){
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"]="FileAppender";
	node["file"]=m_filename;
	if(m_level!=LogLevel::UNKNOW)
		node["level"]=LogLevel::Tostring(m_level);
	if(hasFormatter&&m_formatter){
		node["formatter"]=m_formatter->getPattern();
	}
	std::stringstream ss;
	ss<<node;
	return ss.str();
}
void StdoutAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
	if (level >= m_level){
		MutexType::Lock lock(m_mutex);
		std::string str=m_formatter->format(logger,level,event);
		//std中用cout进行输出
		std::cout<<str;//输出格式化后的event
	}
}

std::string StdoutAppender::toYamlString(){
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"]="StdoutAppender";
	if(m_level!=LogLevel::UNKNOW)
		node["level"]=LogLevel::Tostring(m_level);
	if(hasFormatter&&m_formatter){
		node["formatter"]=m_formatter->getPattern();
	}
	std::stringstream ss;
	ss<<node;
	return ss.str();
}

LogFormatter::LogFormatter(const std::string & pattern):m_pattern(pattern){
	init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event){
	std::stringstream ss;
	for (auto& i : m_items)
	{
		//将每一个item项追加到ss中，然后输出
		i->format(ss,logger,level, event);
	}

	return ss.str();
}

	//%xxx  %xxx{xxx}  %%
// 	格式字符串中包含多种类型：
// %d{%Y-%m-%d %H:%M:%S}，%d指明打印时间，后面紧贴着的{%Y-%m-%d %H:%M:%S}指明具体打印时间的格式
// %p，这种是最常见的，百分号后面带个字符，但必须是上面罗列出的这些才有效
// 非格式的字符，比如[%p]里面的括号，还有%f:%l中的冒号，空格等等，还有%%表示转义的%

void LogFormatter::init() // 字符串解析部分，分成三块 1内容 2参数 3是否是命令
{
	//string  format  type
	//str存放的是解析后的格式和非格式字符串，
		//例如%p，str里存放的就是p这个字符（注意不带百分号）；另外也会存储例如括号[]，冒号:等等这些非格式字符。
	//fmt存储的是{xxx}里的格式xxx，例如%d{%Y-%m-%d %H:%M:%S}，那么fmt存放的就是%Y-%m-%d %H:%M:%S（注意是原原本本的字符串）。
	//type为0表示非格式字符，为1表示为格式字符。

	std::vector<std::tuple<std::string, std::string,int>>vec;// 存放处理结果的串，第一个是命令，第二个是参数，根据第三个判断是否字符串
	//vector存放所有类型的解析
	//size_t last_pos = 0;//size_t为整型类型，可表示任意大小
	std::string nstr; //while循环外使用，存放非格式串
	for (size_t i = 0; i < m_pattern.size(); i++)
	{
		if (m_pattern[i] != '%')
		{
			nstr.append(1, m_pattern[i]);//     不是%，将当前的字符放进     nstr(结果字符串)
			continue;//走下一循环
		}
		//是%,并且不是最后一个字符
		if ((i + 1) < m_pattern.size())
		{
			if (m_pattern[i + 1] == '%')//当有两个%时 表示需要添加一个%字符后面那个%才是需要判断的
			{
				nstr.append(1, '%');
				continue;
			}
		}
		//接下来是仅有一个% 字符的情况，因此直接进行分析
			size_t n = i + 1; //%后的位置
			int fmt_status = 0;// //用来记录大括号{}匹配状态
			size_t fmt_begin = 0;//记录左大括号{ 出现的位置
			std::string str;//内容字符串（%m中的m)
			std::string fmt;//寻找括号，观察里面的参数，对应tuple里的fmt，就是{}里面的内容

			while (n < m_pattern.size()){
				if (fmt_status==0&& (!isalpha(m_pattern[n])//不是英文字母，也不是‘{’‘}’
					&& m_pattern[n] != '{'
					&&m_pattern[n] != '}')) {//过滤字符第一次的时候记录要输出的类型
					//遇到不在大括号内，非字符，非{}，就将[i+1,n)下标区间的格式字符串放入str,长度就是n-(i+1)，所以是substr(i+1,n-i-1)
					str = m_pattern.substr(i + 1, n - i - 1);//一般是检测到了非括号外的%则节取百分号之前上一个%的串
					break;
				}
				if (fmt_status == 0)
				{
					if (m_pattern[n] == '{')
					{
					str = m_pattern.substr(i + 1, n - i-1);//当检测到了{则将百分号后面的字母加入str如：%asv{}中的asv其中v=i+4
					fmt_status = 1;//解析格式
					fmt_begin = n ;
					++n;
					continue;
					}
				}
				if (fmt_status == 1)
				{
					if (m_pattern[n] == '}')
					{
						fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin-1);//fmt记录{  }中的内容
						fmt_status = 0;
						++n;
						break;
					}
				}
				++n;
				if(n==m_pattern.size()){//若到字符串最尾部
					if(str.empty())
						str = m_pattern.substr(i + 1);
				}
			}
			//以i为起点的循环结束，得到一个模式串，对其进行处理
			if (fmt_status == 0)//即，直到遍历到最后都没有遇见{，即为  %xxxxxx
			{
				if (!nstr.empty())
				{
					vec.push_back(make_tuple(nstr,std::string(), 0));
					nstr.clear();
				}
				vec.push_back(make_tuple(str, fmt, 1));//type为1
				i = n-1;
			}
			else if (fmt_status == 1)//只遍历到{，未遍历到}，格式错误
			{
				vec.push_back(make_tuple("<<pattern_error>>", fmt, 0));

				//格式错误
				m_error=true;
				std::cout << "pattern parse error:" << m_pattern << "-" << m_pattern.substr(i) << std::endl;
			}
	}
	if (!nstr.empty())
	{
		vec.push_back(make_tuple(nstr, "", 0));
	}
	static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str,C) \
	{#str,[](const std::string& fmt) {return FormatItem::ptr(new C(fmt)); }}//map存储<string, function>类对象
	//m,[](const std::string& fmt) {return FormatItem::ptr(new MessageFormatItem(fmt))
	XX(m, MessageFormatItem),
	XX(p, LevelFormatItem),
	XX(r, elapseFormatItem),
	XX(c, nameFormatItem),
	XX(t,threadIdFormatItem),
	XX(n, newlineFormatItem),
	XX(d,dataTimeFormatItem),
	XX(f, fileNameFormatItem),
	XX(l,lineFormatItem),
	XX(T,TabFormatItem),
	XX(F,FiberIdFormatItem),
	XX(N, ThreadNameFormatItem),
#undef XX
	};
	// 在定义成功后，比如打印 %t 线程号，那么需要ThreadIdFormatItem，
	//现在vec里就会有一个三元组（“t”,“”,1），那么用三元组的第0位"t"就可以在map中找到ThreadIdFormatItem的智能指针。
	for (auto &i : vec){//解析vec数组，按照顺序放入类内items数组
		//若第i个元素的第三个参数是0，表明不是字符串，直接输出
		if (std::get<2>(i) == 0){//get<2>是vec容器的第三个参数的类型，即int
			m_items.push_back(FormatItem::ptr(new stringFormatItem(std::get<0>(i))));
		}else{
			auto it = s_format_items.find(std::get<0>(i));
			if (it == s_format_items.end()){
				m_items.push_back(FormatItem::ptr(new stringFormatItem("error_format <%" + std::get<0>(i) + ">")));
				//formatter错误，m_error=true;
				m_error=true;
			}
			else{
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
			//std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
	}
    //std::cout << m_items.size() << std::endl;
	//  %m--消息体  
	//  %p--日志level
	//  %r--启动后的时间
	//  %c --日志名称
	//  %t--线程id
	//  %n--回车换行
	//  %d--时间
	//  %f--文件名
	//  %l--行号
}

LoggerManager:: LoggerManager(){
	m_root.reset(new Logger);

	//添加一个默认的appender
	m_root->addAppender(LogAppender::ptr(new StdoutAppender));

	m_loggers[m_root->m_name]=m_root;

	init();
}
Logger::ptr LoggerManager::getLogger(const std::string &name){
	MutexType::Lock lock(m_mutex);
	auto it=m_loggers.find(name);
    if(it!=m_loggers.end()){
		return it->second;
	}
	Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}



struct LogAppenderDefine{
	int type=0;//1、 file   2、 Stdout
	LogLevel::Level level=LogLevel::UNKNOW;
	std::string formatter;
	std::string file;

	bool operator==(const LogAppenderDefine& oth) const{
		return type==oth.type
			&&level==oth.level
			&&formatter==oth.formatter
			&&file==oth.file;
	}
};


struct LogDefine{
	std::string name;//日志器名称
	LogLevel::Level level=LogLevel::UNKNOW;//日志器级别
	std::string formatter;
	std::vector<LogAppenderDefine>appenders;

		bool operator==(const LogDefine& oth) const{
		return name==oth.name
			&&level==oth.level
			&&formatter==oth.formatter
			&&appenders==oth.appenders;
	}

	bool operator<(const LogDefine& oth) const{
		return name<oth.name;
	}

};

template<>
class LexicalCast<std::string,std::set<LogDefine>>{//string转成set<LogDefine> 
public:
    std::set<LogDefine> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    std::set<LogDefine>vec;
	//node["name"].IsDefined();
    for(size_t i=0;i<node.size();++i){
        auto n=node[i];
		if(!n["name"].IsDefined()){//如果没有"name"这个字符串
			std::cout<<"logconfig error: name is null,"<<n
					<<std::endl;
			continue;
		}
		//存在"name"字符串
		LogDefine ld;//创建一个log对象ld
		//if(n["name"].IsScalar())
		ld.name=n["name"].as<std::string>();//ld的名字就是"name"对应的名字

		//级别，string转成LogLevel类型
		ld.level=LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() :"");
		if(n["formatter"].IsDefined()){
			//formatter初始化
			ld.formatter=n["formatter"].as<std::string>();
		}
		if(n["appenders"].IsDefined()){
			//appenders初始化
			for(size_t x=0;x<n["appenders"].size();++x){
				auto a=n["appenders"][x];
				if(!a["type"].IsDefined()){
					std::cout<<"logconfig error: appender type is null,"<<a
							<<std::endl;
					continue;
				}
				std::string type=a["type"].as<std::string>();

				LogAppenderDefine lad;
				if(type=="FileAppender"){
					lad.type=1;
					if(!a["file"].IsDefined()){
					std::cout<<"logconfig error: fileappender file is null,"<<a
							<<std::endl;
					continue;
					}
					lad.file=a["file"].as<std::string>();
					if(a["formatter"].IsDefined()){
						lad.formatter=a["formatter"].as<std::string>();
					}
				}else if(type=="StdoutAppender"){
					lad.type=2;
					if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
				}else{
					std::cout<<"logconfig error: appender type is invalid,"<<a
							<<std::endl;
					continue;
				}
				ld.appenders.push_back(lad);
			}
		}
		vec.insert(ld);
    }
    return vec;
}
};

//set<LogDefine> to string
template<>
class LexicalCast<std::set<LogDefine>,std::string>{
public:
	std::string operator() (const std::set<LogDefine>& v){
    YAML::Node node;
    for(auto& i:v){
        YAML::Node n;
		n["name"]=i.name;
		if(i.level!=LogLevel::UNKNOW){
			n["level"]=LogLevel::Tostring(i.level);	
		}

		if(!i.formatter.empty()){
			n["formatter"]=i.formatter;
		}
		for(auto& a:i.appenders){
			YAML::Node na;
			if(a.type==1){
				na["type"]="FileAppender";
				na["file"]=a.file;
			}else if(a.type==2){
				na["type"]="StdoutAppender";
			}
			if(a.level!=LogLevel::UNKNOW)
				na["level"]=LogLevel::Tostring(a.level);

			if(!a.formatter.empty()){
				na["formatter"]=a.formatter;
			}
			n["appenders"].push_back(na);
		}
		node.push_back(n);
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
};

//创建一个logs    	  名字，默认值（空），描述
sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines=
	sylar::Config::Lookup("logs",std::set<LogDefine>(),"logs config");


//main函数之前执行
struct LogIniter{
	LogIniter(){
		//给名字为logs的配置添加变更回调
		g_log_defines->addListener([](const std::set<LogDefine>& old_value,
						const std::set<LogDefine>& new_value ){
			SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"on_logger_conf_changed";
			 //新增
			for(auto& i:new_value){
				auto it=old_value.find(i);
				sylar::Logger::ptr logger;
				//如果old中没有new的
				if(it==old_value.end()){
					//新增logger
				 	logger=SYLAR_LOG_NAME(i.name);
				}else{
					//如果old中有new的，将old改为new
					if(!(i==*it)){
					//修改logger
					logger=SYLAR_LOG_NAME(i.name);
					}
				}
				 logger->setLevel(i.level);
				if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }
				logger->clearAppenders();
				for(auto& a:i.appenders){
					sylar::LogAppender::ptr ap;
					if(a.type == 1) {
                        ap.reset(new FileAppender(a.file));
                    } else if(a.type==2){
						ap.reset(new StdoutAppender);
					}
					ap->setLevel(a.level);
					
					if(!a.formatter.empty()){
						LogFormatter::ptr fmt(new LogFormatter(a.formatter));
						if(!fmt->isError()){
							ap->setFormatter(fmt);
						}else{
							std::cout<<"log name="<<i.name<<"appender type="<<a.type
									 <<"formatter="<<a.formatter<<"is invaild"<<std::endl;
						}
					}
					logger->addAppender(ap);
				}
			}
			for(auto& i:old_value){
				auto it=new_value.find(i);
				//老的里面有，新的没有--->删除
				if(it==new_value.end()){
					//删除logger
						auto logger=SYLAR_LOG_NAME(i.name);
						//将其日志级别设为100，相当于删除了
						logger->setLevel((LogLevel::Level)100);
						logger->clearAppenders();
				
				}
			}
		});//添加回调结束，这是一个回调函数
	}//构造函数
};//结构体结束

static LogIniter ___log_init;


std::string LoggerManager::toYamlString(){
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	for(auto& i:m_loggers){
		node.push_back(YAML::Load(i.second->toYamlString()));
	}
	std::stringstream ss;
	ss<<node;
	return ss.str();
}
void LoggerManager::init()
{

}



}