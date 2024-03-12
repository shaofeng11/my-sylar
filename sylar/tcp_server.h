#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include<memory>
#include<functional>
#include"iomanager.h"
#include"socket.h"
#include"address.h"
#include"noncopyable.h"
namespace sylar{

class TcpServer :public std::enable_shared_from_this<TcpServer>
                            ,Noncopyable{
public:
    typedef std::shared_ptr<TcpServer>ptr;

    TcpServer(sylar::IOManager* worker=sylar::IOManager::GetThis()
                ,sylar::IOManager* aworker=sylar::IOManager::GetThis());
    virtual ~TcpServer();

    //服务器端bind一个地址
    virtual bool bind(sylar::Address::ptr addr);

    //bind多个地址，addrs是要bind的地址数组，fails时bind失败的地址数组
    virtual bool bind(const std::vector<Address::ptr>& addrs,
                        std::vector<Address::ptr>& fails);

    virtual bool start();
    virtual void stop();

    uint64_t getReadTimeout() const { return m_recvTimeout;}
    std::string getName() const{ return m_name;}

    void setReadTimeout(uint64_t v) { m_recvTimeout=v;}
    void setName(const std::string& v) {  m_name=v;}

    bool isStop() const{ return m_isStop;}

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr>m_socks;//用来监听
    IOManager* m_worker; //相当于一个线程池，工作线程池，给accept之后的socket使用
    IOManager* m_acceptWorker;//accept线程
    uint64_t m_recvTimeout;
    std::string m_name;//服务器名称
    bool m_isStop;
};

}




#endif