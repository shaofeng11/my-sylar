#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__
#include"stream.h"
#include"socket.h"

namespace sylar{

class SocketStream :public Stream{
public:
    typedef std::shared_ptr<SocketStream>ptr;
    //owner表示socket是不是全权交给socketstream管理
    SocketStream(Socket::ptr sock,bool owner=true);
    ~SocketStream();
    int read(void* buffer,size_t length) override;
    int read(sylar::ByteArray::ptr ba,size_t length) override;
    int write(const void* buffer,size_t length) override;
    int write(sylar::ByteArray::ptr ba,size_t length) override;
    virtual void close() override;

    Socket::ptr getSocket() const {return m_socket;}

    bool isConnected()const;

protected:
    Socket::ptr m_socket;
    bool m_owner;
};

}



#endif