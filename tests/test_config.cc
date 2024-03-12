#include "sylar/config.h"
#include "sylar/log.h"
#include <yaml-cpp/yaml.h>
#include <string.h>
#include<map>
#include<iostream>

#if l
//int 类型       多态，父类指针指向子类对象
sylar::ConfigVar<int>::ptr g_int_value_config=
    sylar::Config::Lookup("system.port",(int)8080,"system port"); //创建一个配置    int类型
//sylar::ConfigVar<float>::ptr g_int_valuex_config=
    //sylar::Config::Lookup("system.port",(float)8080,"system port"); //创建一个配置    int类型
//float类型
sylar::ConfigVar<float>::ptr g_float_value_config=
    sylar::Config::Lookup("system.value",(float)10.2f,"system value");
//vector类型
sylar::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config=
    sylar::Config::Lookup("system.int_vec",std::vector<int>{1,2},"system int vector");
//list类型
 sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config=
    sylar::Config::Lookup("system.int_list",std::list<int>{1,2},"system int list");   
//set类型
 sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config=
    sylar::Config::Lookup("system.int_set",std::set<int>{1,2},"system int set"); 
//unordered_set类型
 sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config=
    sylar::Config::Lookup("system.int_uset",std::unordered_set<int>{1,2},"system int uset");     
 //map类型
 //key必须是string类型，value可以是复杂类型
 sylar::ConfigVar<std::map<std::string,int>>::ptr g_str_int_map_value_config=
    sylar::Config::Lookup("system.str_int_map",std::map<std::string,int>{{"k",2}},"system str int map");  
 //unordered_map类型
 sylar::ConfigVar<std::unordered_map<std::string,int>>::ptr g_str_int_umap_value_config=
    sylar::Config::Lookup("system.str_int_umap",std::unordered_map<std::string,int>{{"k",2}},"system str int umap");   
void print_yaml(const YAML::Node& node,int level){
    if(node.IsScalar()){//Scalar 标量，对应YAML格式中的常量
    //std::string(level * 4, ' ')  四个缩进
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level * 4, ' ')
                                         <<node.Scalar()<<"-"<<node.Type()<<"-"<<level;
    }else if(node.IsNull()){//Null 空节点
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level * 4, ' ')
                                         <<"NULL -"<<node.Type()<<"-"<<level;
    }else if(node.IsMap()){//Map 类似标准库中的Map，对应YAML格式中的对象
        for(auto it=node.begin();it!=node.end();it++){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level * 4, ' ')
                                             <<it->first<<"-"<<it->second.Type()<<"-"<<level; 
            print_yaml(it->second,level+1);
        }
    }else if(node.IsSequence()){//Sequence 序列，类似于一个Vector,对应YAML格式中的数组
       for(size_t i=0;i<node.size();++i){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level * 4, ' ')
                                            <<i<<"-"<<node[i].Type()<<"-"<<level;  
            print_yaml(node[i],level+1);  
       }
    }
}

void test_yaml(){ 
    YAML::Node root = YAML::LoadFile("/home/shaofeng/workspace/bin/conf/log.yml");
    print_yaml(root,0); 

    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<root;
}

void test_config(){
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before"<<g_float_value_config->toString();

#define XX(g_var,name,prefix) \
{ \
    auto& v=g_var->getValue(); \
    for(auto& i:v){ \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" " #name":"<<i; \
    } \
     SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" " #name" yaml:"<<g_var->toString(); \
} 

#define XX_M(g_var,name,prefix) \
{ \
    auto& v=g_var->getValue(); \
    for(auto& i:v){ \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" " #name":{" \
                                        <<i.first<<"--"<<i.second<<"}";\
    } \
     SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix" " #name" yaml:"<<g_var->toString(); \
}
    XX(g_int_vector_value_config,int_vec,before);
    XX(g_int_list_value_config,int_list,before);
    XX(g_int_set_value_config,int_set,before);
    XX(g_int_uset_value_config,int_uset,before);
    XX_M(g_str_int_map_value_config,str_int_map,before);
    XX_M(g_str_int_umap_value_config,str_int_umap,before);


    YAML::Node root = YAML::LoadFile("/home/shaofeng/workspace/bin/conf/test.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after"<<g_float_value_config->toString();

   
    XX(g_int_vector_value_config,int_vec,after);
    XX(g_int_list_value_config,int_list,after);
    XX(g_int_set_value_config,int_set,after);
    XX(g_int_uset_value_config,int_uset,after);
    XX_M(g_str_int_map_value_config,str_int_map,after);
    XX_M(g_str_int_umap_value_config,str_int_umap,after);
}

#endif
class Person{
public:
    std::string m_name;
    int m_age=0;
    bool m_sex=0;
    
    std::string toString()const{
        std::stringstream ss;
        ss<<"[Person name="<<m_name
            <<"  age="<<m_age
            <<"  sex="<<m_sex
            <<"]";
        return ss.str();
    }

    bool operator==(const Person& oth) const{
        return m_name==oth.m_name
                &&m_age==oth.m_age
                &&m_sex==oth.m_sex;
    }
};

namespace sylar{
template<>
class LexicalCast<std::string,Person>{
public:
    Person operator() (const std::string& v){
    YAML::Node node= YAML::Load(v);

    Person p;
    p.m_name=node["name"].as<std::string>();
    p.m_age=node["age"].as<int>();
    p.m_sex=node["sex"].as<bool>();

    return p;
}
};

//person to string
template< >
class LexicalCast<Person,std::string>{
public:
std::string operator() (const Person& p){
    YAML::Node node;
    node["name"]=p.m_name;
    node["age"]=p.m_age;
    node["sex"]=p.m_sex;
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
};
}

//自定义类型
sylar::ConfigVar<Person>::ptr g_Person=
    sylar::Config::Lookup("class.person",Person(),"system person");

//map嵌套person
sylar::ConfigVar<std::map<std::string,Person>>::ptr g_Person_map=
    sylar::Config::Lookup("class.map",std::map<std::string,Person>(),"system person");

//map中嵌套vector，vector中嵌套person
sylar::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    sylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");


void test_class(){
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before"<<g_Person->getValue().toString()<<"-"<<g_Person->toString();
#define XX_PM(g_var,prefix) \
   { \
    auto m=g_Person_map->getValue(); \
    for(auto& i:m){ \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<prefix<<":"<<i.first<<"-"<<i.second.toString(); \
    } \
     SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<prefix<<": size="<<m.size();  \
   }

    g_Person->addListener([](const Person& old_value,const Person& new_value){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"old_value="<<old_value.toString()
                                        <<"new_value="<<new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_vec_map->toString();
    YAML::Node root = YAML::LoadFile("/home/shaofeng/workspace/bin/conf/test.yml");
    sylar::Config::LoadFromYaml(root);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"after"<<g_Person->getValue().toString()<<"-"<<g_Person->toString();
    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log(){
    static sylar::Logger::ptr system_log=SYLAR_LOG_NAME("system");
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;

    std::cout<<sylar::LoggerMgr::GetInstance()->toYamlString()<<std::endl;
    YAML::Node root=YAML::LoadFile("/home/shaofeng/workspace/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);
    std::cout<<"--------------"<<std::endl;
    std::cout<<sylar::LoggerMgr::GetInstance()->toYamlString()<<std::endl;
    std::cout<<"--------------"<<std::endl;
    std::cout<<root<<std::endl;
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;

    system_log->setFormatter("%d - %m%n");
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl;
}

int main()
{
   // SYLAR_LOG_INFO(SYLAR_LOG_ROOT());
    //test_class();
    //test_config();
    //test_yaml();
    //test_log();
      sylar::Config::Visit([](sylar::ConfigVarBase::ptr var){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"name="<<var->getName()
                                    <<" description="<<var->getDescription()
                                    <<" typename="<<var->getTypeName()
                                    <<" val="<<var->toString();
        });   
    //test_config();
    return 0;
}