#include "../sylar/http/http_server.h"
#include"../sylar/log.h"

static sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();
void run(){
    // 先创建一个HttpServer，将该server   bind一个本机地址，然后listen，accept()等待有client连接。
    // 向该server中的ServletDispatch添加一个servlet。然后调用server的start()方法。
	// 当有一个请求过来时，start中调用的startAccept方法会创建一个名为client的socket，然后执行server的handclient。
	// HttpServer的handleClient中，用client创建一个HttpSession，调用session中的recvRequest()方法接受客户端的请求包，
    // 然后调用session的m_dispatch的handle函数处理请求包(用添加的Servlet处理)并做出回应，然后用session的sendResponse向请求方发送一个回应。

    sylar::http::HttpServer::ptr server(new sylar::http::HttpServer);
    sylar::Address::ptr addr=sylar::Address::LookupAnyIpAddress("0.0.0.0:8020");
    while(!server->bind(addr)){
        sleep(2);
    }
    auto sd=server->getServletDispatch();
    sd->addServlet("/sylar/xx",[](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session){
        //rsp->setBody(req->toString());
        rsp->setBody("wo shi shao feng");
        return 0;
    });

    sd->addGlobServlet("/sylar/*",[](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session){
        rsp->setBody("Glob:\r\n"+req->toString());
        return 0;
    });
    server->start();
}
int main(int argc,char** argv){
    sylar::IOManager iom(2);
    iom.schedule(run);
}