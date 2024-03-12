#include "servlet.h"
#include<fnmatch.h>

namespace sylar{
namespace http{
FunctionServlet::FunctionServlet(callback cb)
        :Servlet("FunctionServlet")
        ,m_cb(cb){
}

//直接执行回调
int32_t FunctionServlet::handle(sylar::http::HttpRequest::ptr request
                ,sylar::http::HttpResponse::ptr response
                ,sylar::http::HttpSession::ptr session){
    return m_cb(request,response,session);                    
}


ServletDispath::ServletDispath()
    :Servlet("ServletDispatch"){
        m_default.reset(new NotFoundServlet());
}

int32_t ServletDispath::handle(sylar::http::HttpRequest::ptr request
                ,sylar::http::HttpResponse::ptr response
                ,sylar::http::HttpSession::ptr session){
    auto slt=getMatchedServlet(request->getPath()); //
    if(slt){
        slt->handle(request,response,session);
    }
    return 0;
}

void ServletDispath::addServlet(const std::string uri,Servlet::ptr slt){
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri]=slt;
}

void ServletDispath::addServlet(const std::string uri,FunctionServlet::callback cb){
    RWMutexType::WriteLock lock(m_mutex);
    //将cb封装到一个新的servlet
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispath::addGlobServlet(const std::string& uri,Servlet::ptr slt){
    RWMutexType::WriteLock lock(m_mutex);
    //如果有了，先将原来的删除，再添加新的
    for(auto it=m_globs.begin();it!=m_globs.end();it++){
        if(it->first==uri){
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri,slt));
}
void ServletDispath::addGlobServlet(const std::string& uri,FunctionServlet::callback cb){
    return addGlobServlet(uri,FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispath::delServlet(const std::string& uri){
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}
void ServletDispath::delGlobalServlet(const std::string& uri){
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it=m_globs.begin();it!=m_globs.end();it++){
        if(it->first==uri){
            m_globs.erase(it);
            break;
        }
    }
}
Servlet::ptr ServletDispath::getServlet(const std::string& uri){
    RWMutexType::ReadLock lock(m_mutex);
    auto it=m_datas.find(uri);
    return it==m_datas.end() ? nullptr :it->second;

}
Servlet::ptr ServletDispath::getGlobalServlet(const std::string& uri){
    RWMutexType::ReadLock lock(m_mutex);
    for(auto it=m_globs.begin();it!=m_globs.end();it++){
        if(it->first==uri){
            return it->second;
        }
    }
    return nullptr;
}

Servlet::ptr ServletDispath::getMatchedServlet(const std::string& uri){
    //先在m_datas里找uri，找到直接返回对应的servlet
    RWMutexType::ReadLock lock(m_mutex);
    auto it=m_datas.find(uri);
    if(it!=m_datas.end()){
        return it->second;
    }
    //未找到，在m_globs里找
    for(auto it=m_globs.begin();it!=m_globs.end();it++){
        if(!fnmatch(it->first.c_str(),uri.c_str(),0)){
            return it->second;
        }
    }
    //都为找到，返回默认，NotFound
    return m_default;
}

NotFoundServlet::NotFoundServlet()
    :Servlet("NotFoundServlet"){

}
int32_t NotFoundServlet::handle(sylar::http::HttpRequest::ptr request
                ,sylar::http::HttpResponse::ptr response
                ,sylar::http::HttpSession::ptr session){
    static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>sylar/1.0.0</center></body></html>";
    response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
    response->setBody(RSP_BODY);
    response->setHeader("Server", "sylar/1.0.0");
    response->setHeader("Content-Type", "text/html");
    return 0;
}   
}
}
