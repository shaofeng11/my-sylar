#include "../sylar/log.h"
#include"../sylar/address.h"

sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

void test(){
    SYLAR_LOG_INFO(g_logger)<<"begin";
    //创建一个vector，存放Address
    std::vector<sylar::Address::ptr>addrs;

    bool v=sylar::Address::Lookup(addrs,"www.baidu.com:ftp");
    if(!v){
        SYLAR_LOG_ERROR(g_logger)<<"lookup fail";
    }
    for(size_t i=0;i<addrs.size();++i){
        SYLAR_LOG_INFO(g_logger)<<i<<" - "<<addrs[i]->toString();
    }
    SYLAR_LOG_INFO(g_logger)<<"end";
}

void test_lookup(){
      SYLAR_LOG_INFO(g_logger)<<"begin";
    //创建一个vector，存放Address
    std::vector<sylar::Address::ptr>addrs;


    //任意返回一个地址
    sylar::IPAddress::ptr v=sylar::Address::LookupAnyIpAddress("www.baidu.com:80");
    if(!v){
        SYLAR_LOG_ERROR(g_logger)<<"lookup fail";
    }
    SYLAR_LOG_INFO(g_logger)<<v->toString();
    SYLAR_LOG_INFO(g_logger)<<"end";
}

void test_iface(){
    std::multimap<std::string,std::pair<sylar::Address::ptr,uint32_t>>results;
    bool v=sylar::Address::GetInterfaceAddresses(results);
    if(!v){
        SYLAR_LOG_ERROR(g_logger)<<"GetInterfaceAddress fail";
        return ;
    }
    for(auto& i:results){
        SYLAR_LOG_INFO(g_logger)<<i.first<<" - "<<i.second.first->toString()<<" - "
                <<i.second.second;
    }
}

void test_ipv4(){
    //auto addr=sylar::IPAddress::Create("www.sylar.top");
    auto addr=sylar::IPAddress::Create("127.0.0.8");
    if(addr){
        SYLAR_LOG_INFO(g_logger)<<addr->toString();
    }
}
int main(){
    //test_lookup();
    //test_ipv4();
    test_iface();
    //test();
    return 0;
}