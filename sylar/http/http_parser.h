#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__
#include"http.h"
#include"http11_parser.h"
#include"httpclient_parser.h"

namespace sylar{
namespace http{
class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser>ptr;
    HttpRequestParser();

        /**
     * @brief 解析协议
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @return 返回实际解析的长度,并且将已解析的数据移除
     */
    size_t excute(char* data,size_t len);
    int isFinish();
    int hasError() ;
    HttpRequest::ptr getData() const { return m_data;}
    void setError(int v){ m_error=v;}

    uint64_t getContentLength();
    const http_parser& getParser() const {return m_parser;}
public:
    //header大小
    static uint64_t GetHttpRequestBufferSize();
    //body大小
    static uint64_t GetHttpRequestMaxBodyize();
private:

    http_parser m_parser;

    //请求的结构体，解析过后放到这个里面
    HttpRequest::ptr m_data;

    //1000:invalid method
    //1001 invalid version
    //1002 invalid field
    int m_error;
};

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser>ptr;
    HttpResponseParser();
    size_t excute(char* data,size_t len,bool chunck);
    int isFinish();
    int hasError();

    HttpResponse::ptr getData() const { return m_data;}
    void setError(int v){ m_error=v;}

    uint64_t getContentLength();
    const httpclient_parser& getParser() const {return m_parser;}

public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodyize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};

}
}
#endif