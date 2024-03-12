#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__
#include<memory>
#include<functional>
#include<vector>
#include<unordered_map>
#include<string>
#include"http.h"
#include"http_session.h"
#include"../mutex.h"
namespace sylar{
namespace http{

class Servlet{
public:
    typedef std::shared_ptr<Servlet>ptr;
    Servlet(const std::string& name)
        :m_name(name){}
    virtual ~Servlet(){}
    /**
     * @brief 处理请求
     * @param[in] request HTTP请求
     * @param[in] response HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                    ,sylar::http::HttpResponse::ptr response
                    ,sylar::http::HttpSession::ptr session)=0;//作用：根据request填充response
    const std::string& getName() const { return m_name;}
protected:
    std::string m_name;
};


class FunctionServlet : public Servlet{
public:
    typedef std::shared_ptr<FunctionServlet>ptr;
    // 函数回调类型定义
    typedef std::function<int32_t(
        sylar::http::HttpRequest::ptr request
        ,sylar::http::HttpResponse::ptr HttpResponse
        ,sylar::http::HttpSession::ptr session)>callback;

    FunctionServlet(callback cb);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                    ,sylar::http::HttpResponse::ptr response
                    ,sylar::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};


//管理、维护servlet之间的关系
class ServletDispath : public Servlet{
public:
    typedef std::shared_ptr<ServletDispath>ptr;
    typedef RWMutex RWMutexType;

    ServletDispath();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                    ,sylar::http::HttpResponse::ptr response
                    ,sylar::http::HttpSession::ptr session) override;

    void addServlet(const std::string uri,Servlet::ptr slt);

    void addServlet(const std::string uri,FunctionServlet::callback cb);

    void addGlobServlet(const std::string& uri,Servlet::ptr slt);
    void addGlobServlet(const std::string& uri,FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobalServlet(const std::string& uri);

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobalServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}
    void setDefault(Servlet::ptr slt) { m_default=slt;}

    Servlet::ptr getMatchedServlet(const std::string& uri);

private:

    RWMutexType m_mutex;
    //uri(sylar/xxx) ->servlet
    std::unordered_map<std::string,Servlet::ptr>m_datas;

    //uri(sylar/*) ->servlet
    std::vector<std::pair<std::string,Servlet::ptr>>m_globs;;

    //默认servlet，所有路径都没匹配到时使用
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet{
public:
    typedef std::shared_ptr<NotFoundServlet>ptr;
    NotFoundServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                    ,sylar::http::HttpResponse::ptr response
                    ,sylar::http::HttpSession::ptr session) override;
};

}
}


#endif