#include"../sylar/socket.h"
#include"../sylar/sylar.h"
#include"../sylar/iomanager.h"
#include"../sylar/address.h"

static sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();

void test_socket(){
    //首先获取百度的ipv4地址
    sylar::IPAddress::ptr addr=sylar::Address::LookupAnyIpAddress("www.baidu.com");
    if(addr){
        SYLAR_LOG_INFO(g_logger)<<"get address:"<<addr->toString();  
    }else{
        SYLAR_LOG_ERROR(g_logger)<<"get address fail";
        return;
    }
    //用该地址创建一个TCPsock
    sylar::Socket::ptr sock=sylar::Socket::CreateTCP(addr);
    //设置端口  uint16_t
    addr->setPort(80);
    SYLAR_LOG_INFO(g_logger)<<"addr="<<addr->toString();

    //用sock连接百度的addr
    if(!sock->connect(addr)){
        SYLAR_LOG_ERROR(g_logger)<<"connect "<<addr->toString()<<" fail";
    }else{
        SYLAR_LOG_INFO(g_logger)<<"connect "<<addr->toString()<<" connected"; 
    }
    //向addr发送GET / HTTP/1.0\r\n\r\n
    const char buff[]="GET / HTTP/1.0\r\n\r\n";
    int rt=sock->send(buff,sizeof(buff));
    if(rt<=0){
        SYLAR_LOG_INFO(g_logger)<<"send fail rt="<<rt;
        return;
    }

    //接收数据
    std::string buffs;
    buffs.resize(4096);
    rt=sock->recv(&buffs[0],buffs.size());
    if(rt<=0){
        SYLAR_LOG_INFO(g_logger)<<"recv fail rt="<<rt;
        return;
    }
    buffs.resize(rt);
    SYLAR_LOG_INFO(g_logger)<<buffs;
}

int main(int argc,char** argv){
    sylar::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}