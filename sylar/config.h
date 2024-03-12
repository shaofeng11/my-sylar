#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include"thread.h"
#include "log.h"
#include "util.h"

namespace sylar{


//配置变量的基类
    //有name、description两个成员，两个方法，
    //两个纯虚函数将参数值转换成 YAML String 和从 YAML String 转成参数的值。
class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase>ptr;
    // name 配置参数名称[0-9a-z_.]
    //description 配置参数描述
    ConfigVarBase(const std::string& name,const std::string& description="")
        :m_name(name)
        ,m_description(description){
        //在给name初始化后将其转为小写
        std::transform(m_name.begin(),m_name.end(),m_name.begin(),::tolower);
    }
    virtual ~ConfigVarBase(){}

    const std::string& getName()  const {return m_name;}

    const std::string& getDescription() const{return m_description;}

    //转成字符串

    //两个纯虚函数将参数值转换成 YAML String 和从 YAML String 转成参数的值。
    virtual std::string toString()=0;

    virtual bool fromString(const std::string& val)=0;

    virtual std::string getTypeName() const =0;

protected:
    std::string m_name;
    std::string m_description;

};
//F from_type       T:to_type
template<class F,class T>
//通用类型转换
class LexicalCast{//BasicFromStringCast        
    public:
        T operator()(const F& v){//函数调用运算符()重载,也称仿函数
        //boost 传入F类型，返回T类型
            return boost::lexical_cast<T>(v);
        }
};


//偏特化

//string to vector<T>
template<class T>
class LexicalCast<std::string,std::vector<T>>{//将F换成了string
public:
    std::vector<T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
        YAML::Node node= YAML::Load(v);//打开加载一个yml文件

        //模板在实例化之前并不知道std::vector<T>是什么东西，使用typename可以让定义确定下来
        //使用typename告诉编译器std::vector<T>是一个类型而不是变量
        typename std::vector<T>vec;
        //std::vector<T>vec;
        std::stringstream ss;
        for(size_t i=0;i<node.size();++i){
            ss.str("");
            ss<<node[i];
            vec.push_back(LexicalCast<std::string,T>() (ss.str()));//递归
        }
        return vec;
    }
};
//vector<T> to string
template<class T>
class LexicalCast<std::vector<T>,std::string>{//将F换成了string
public:
    std::string operator() (const std::vector<T>& v){//重载函数调用，传入string类型，传出vector<T>类型
        YAML::Node node;
        for(auto& i:v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};


//list容器的偏特化
template<class T>
class LexicalCast<std::string,std::list<T>>{//将F换成了string
public:
std::list<T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    typename std::list<T> vec;
    std::stringstream ss;
    for(size_t i=0;i<node.size();++i){
        ss.str("");
        ss<<node[i];
        vec.push_back(LexicalCast<std::string, T>() (ss.str()));//递归
    }
    return vec;
}
};
template<class T>
class LexicalCast<std::list<T>,std::string>{//将F换成了string
public:
std::string operator() (const std::list<T>& v){//重载函数调用，传入string类型，传出list<T>类型
    YAML::Node node;
    for(auto& i:v){
        node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}
};


//set容器的偏特化
template<class T>
class LexicalCast<std::string,std::set<T>>{//将F换成了string
public:
    std::set<T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    typename std::set<T> vec;
    std::stringstream ss;
    for(size_t i=0;i<node.size();++i){
        ss.str("");
        ss<<node[i];
        vec.insert(LexicalCast<std::string,T>() (ss.str()));//递归
    }
    return vec;
}
};

//set to string
template<class T>
class LexicalCast<std::set<T>,std::string>{//将F换成了string
public:
std::string operator() (const std::set<T>& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node;
    for(auto& i:v){
        node.push_back(YAML::Load( LexicalCast<T,std::string>()(i)));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
};

//unordered_set
template<class T>
class LexicalCast<std::string,std::unordered_set<T>>{//将F换成了string
public:
    std::unordered_set<T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    typename std::unordered_set<T> vec;
    std::stringstream ss;
    for(size_t i=0;i<node.size();++i){
        ss.str("");
        ss<<node[i];
        vec.insert(LexicalCast<std::string,T>() (ss.str()));//递归
    }
    return vec;
}
};

//unordered_set to string
template<class T>
class LexicalCast<std::unordered_set<T>,std::string>{//将F换成了string
public:
std::string operator() (const std::unordered_set<T>& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node(YAML::NodeType::Sequence);
    for(auto& i:v){
        node.push_back(YAML::Load( LexicalCast<T,std::string>()(i)));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
};

//map
template<class T>
class LexicalCast<std::string,std::map<std::string,T>>{//将F换成了string
public:
    std::map<std::string,T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    typename std::map<std::string,T> vec;
    std::stringstream ss;
    for(auto it=node.begin();it!=node.end();it++){
        ss.str("");
        ss<<it->second;
        vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>() (ss.str())));//递归
    }
    return vec;
}
};

//map to string
template<class T>
class LexicalCast<std::map<std::string,T>,std::string>{//将F换成了string
public:
std::string operator() (const std::map<std::string,T>& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node(YAML::NodeType::Sequence);
    for(auto& i:v){
        node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
};

//unordered_map
template<class T>
class LexicalCast<std::string,std::unordered_map<std::string,T>>{//将F换成了string
public:
    std::unordered_map<std::string,T> operator() (const std::string& v){//重载函数调用，传入string类型，传出vector<T>类型
    YAML::Node node= YAML::Load(v);

    typename std::unordered_map<std::string,T> vec;
    std::stringstream ss;
    for(auto it=node.begin();it!=node.end();it++){
        ss.str("");
        ss<<it->second;
        vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>() (ss.str())));//递归
    }
    return vec;
}
};

//map to string
template<class T>
class LexicalCast<std::unordered_map<std::string,T>,std::string>{//将F换成了string
public:
    std::string operator() (const std::unordered_map<std::string,T>& v){//重载函数调用，传入string类型，传出vector<T>类型
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i:v){
            node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

/**
 * @brief 配置参数模板子类,保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */

//FromStr T operator(const std::string&)从std::string转换成T类型的仿函数
//ToStr   std::string operator(){const T&}
template<class T,class FromStr=LexicalCast<std::string,T>
                ,class ToStr=LexicalCast<T,std::string> >
//子类重写tostring和fromstring，还有一个成员m_val
class ConfigVar:public ConfigVarBase{
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar>ptr;


    //回调函数 
    //看不懂
    //返回值void,参数为const T类型
    typedef std::function<void(const T& old_value,const T& new_value)> on_change_cb;

    ConfigVar (const std::string& name
            ,const T& defult_value
            ,const std::string & description="")
        :ConfigVarBase(name,description)
        ,m_val(defult_value){

    }

    std::string toString() override{
        try{
            RWMutexType::ReadLock lock(m_mutex);
           //return boost::lexical_cast<std::string>(m_val); 
            return ToStr()(m_val);//ToStr=Lexicalcast<int/float/vector<int/float/......>/set<int/float/......>/list/map<int/string>,std::string>

           //exception 当转换失败抛出异常
        }catch (std::exception& e){
           //exception是一个异常，what成员函数返回字符串形式的异常描述信息
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::toString exception"
            <<e.what()<<"   convert:"<<typeid(m_val).name()<<"  to string";
            //typeid(m_val).name()------>>>返回m_val的类型名（模板T类型）
        }
        return "";
    }

     bool fromString(const std::string& val) override{
        try{
          // m_val=boost::lexical_cast<T>(val); 
          setValue(FromStr()(val));//匿名函数对象的写法
        }catch(std::exception& e){
         std::cout<<"0"<<std::endl;
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::fromString exception"
            <<e.what()<<"   convert: string to"<<typeid(m_val).name()
            <<" - "<<val;
        }
        return false;
     }
    const T getValue()  {
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }
    void setValue(const T& v){
        {
            //中括号，给lock加一个作用域，大括号结束，lock在析构函数中被释放
            RWMutexType::ReadLock lock(m_mutex);
            if(v==m_val){
                return;
            }
            for(auto& i:m_cbs){
                i.second(m_val,v);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val=v;
    }
    std::string getTypeName() const override {return typeid(T).name();}

    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id=0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id]=cb;
        return s_fun_id;
    }

    void delListener(uint64_t key){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    void clearListener(){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }
    on_change_cb getListener(uint64_t key){
        RWMutexType::ReadLock lock(&m_mutex);
        auto it=m_cbs.find(key);
        return it==m_cbs.end() ? nullptr : it->second;
    }
private:
    RWMutexType m_mutex;
    T m_val;

    //变更回调函数组 unit64_t key,要求唯一，一般可以用hash
    std::map<uint64_t,on_change_cb>m_cbs;//（key  回调函数的id  用于删除回调)
};



/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config {
public:
    typedef RWMutex RWMutexType;
    //map<配置名,具体配置（基类）>
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    template<class T>
    //创建
    //这里的typename是告诉编译器这是类型，因为在有些情况下::后面是变量名
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
        const T& default_value, const std::string& description = ""){
        RWMutexType::WriteLock lock(GetMutex());
        //处理map key相同时不报错
        auto it=GetDatas().find(name);
        if(it!=GetDatas().end()){
            //基类转成子类
            auto tmp=std::dynamic_pointer_cast<ConfigVar<T>>(it->second);//转成智能指针
            //如果类型不一样，智能指针会返回nullptr
            if(tmp){
                //已存在
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<" exists";
                return tmp;
            }else{
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<" exists but type not "
                                                <<typeid(T).name()<<" real_type="<<it->second->getTypeName()
                                                <<" "<<it->second->toString();
                return nullptr;
            }
        }

        //没找到，创建一个新的配置
        //如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                                !=std::string::npos){
            //输出错误日志，抛出异常
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name invalid   "<<name;
            throw std::invalid_argument(name);
        }
        //创建，放到s_datas里
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        GetDatas()[name]=v;
        //返回新创建的
        return v;
    }
    template<class T>
    //查找配置参数,返回配置参数的具体子类
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it=GetDatas().find(name);
        if(it==GetDatas().end()){
            return nullptr;
        }
        //若找到，返回该类型的智能指针 ,将父类的智能指针强转成子类的智能指针
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static void LoadFromYaml(const YAML::Node& root);

     //查找配置参数,返回配置参数的基类
    static ConfigVarBase::ptr LookupBase(const std::string& name);

    static void Visit(std::function<void(ConfigVarBase::ptr)>cb);
private:
//静态方法返回静态局部变量
     static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
         return s_datas;
     }
    static RWMutexType& GetMutex(){
        static RWMutexType s_mutex;
        return s_mutex;
    }
};
}
#endif