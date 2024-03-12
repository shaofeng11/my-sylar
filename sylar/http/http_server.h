#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__
#include<memory>
#include"http_session.h"
#include"../tcp_server.h"
#include"servlet.h"


namespace sylar{
namespace http{
class HttpServer : public TcpServer{
public:
    typedef std::shared_ptr<HttpServer>ptr; 
    HttpServer(bool keepalive=false,
                sylar::IOManager* worker=sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker=sylar::IOManager::GetThis());
    ServletDispath::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(ServletDispath::ptr v){ m_dispatch=v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepalive;
    ServletDispath::ptr m_dispatch;    
};


}
}

#endif