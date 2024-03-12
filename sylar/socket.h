#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__
#include<memory>
#include"address.h"
#include"noncopyable.h"

namespace sylar{

class Socket:public std::enable_shared_from_this<Socket>,Noncopyable{
public:
    typedef std::shared_ptr<Socket>ptr;
    typedef std::weak_ptr<Socket>wear_ptr;

    enum Type{
        TCP=SOCK_STREAM,
        UDP=SOCK_DGRAM
    };

    enum Family{
        IPv4=AF_INET,
        IPv6=AF_INET6,
        UNIX=AF_UNIX
    };

    //创造TCP/UDP socket
    static Socket::ptr CreateTCP(sylar::Address::ptr address);
    static Socket::ptr CreateUDP(sylar::Address::ptr address);

    //创建IPv4的TCP Socket
    static Socket::ptr CreateTCPSocket();
    //创建IPv4的UDP Socket
    static Socket::ptr CreateUDPSocket();
    //创建IPv6的TCP Socket
    static Socket::ptr CreateTCPSocket6();
    //创建IPv6的UDP Socket
    static Socket::ptr CreateUDPSocket6();
    //创建Unix的TCP Socket
    static Socket::ptr CreateUnixTCPSocket();
    //创建Unix的UDP Socket
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family,int type,int protocol=0);
    ~Socket();

    //获取发送超时时长
    int64_t getSendTimeout();
    //设置超时时长
    void setSendTimeout(int64_t v);

    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    //获取socket   (getsockopt)
    bool getOption(int level,int option,void* result,socklen_t* len);
    template<class T>
    bool getOption(int level,int option,T& result){
        socklen_t length=sizeof(T);
        return getOption(level,option,&result,&length);
    }

    bool setOption(int level,int option,const void* result,socklen_t len);
    template<class T>
    bool setOption(int level,int option,const T& value){
        return setOption(level,option,&value,sizeof(T));
    }
    /**
     * @brief 接收connect链接
     * @return 成功返回新连接的socket,失败返回nullptr
     * @pre Socket必须 bind , listen  成功
     */
    Socket::ptr accept();

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    bool bind(const Address::ptr addr);
    bool connect(const Address::ptr addr,uint64_t timeout_ms=-1);
    bool listen(int backlog=SOMAXCONN);
    bool close();

    //TCP用
    int send(const void* buffer,size_t length,int flags=0);
    int send(const iovec* buffers,size_t length,int flags=0);

    //UDP用
    int sendTo(const void* buffer,size_t length,const Address::ptr to,int flags=0);
    int sendTo(const iovec* buffers,size_t length,const Address::ptr to,int flags=0);

    //TCP用
    int recv(void* buffer,size_t length,int flags=0);
    int recv(iovec* buffers,size_t length,int flags=0);

    //UDP用
    int recvFrom(void* buffer,size_t length,Address::ptr from,int flags=0);
    int recvFrom(iovec* buffers,size_t length,Address::ptr from,int flags=0);

    //远端连的ip
    Address::ptr getRomoteAddress();
    //本地用的ip
    Address::ptr getLocalAddress();

    int getFamily() const{ return m_family;}
    int getType() const {return m_type;}
    int getProtocol() const {return m_protocol;}

    bool isConnected() const {return m_isConnected;}
    bool isValid() const;
    int getError();

    std::ostream& dump(std::ostream& os) const;

    //返回socket句柄
    int getSocket() const {return m_sock;}

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();
private:
    void initSock();
    void newSock();

    bool init(int sock);
private:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    int m_isConnected;

    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;

};

std::ostream& operator<<(std::ostream& os,const Socket& addr);
}
#endif