#include "http.h"
#include<map>
#include<string.h>
namespace sylar{
namespace http{

//将字符串方法名转成HTTP方法枚举
//如果m和Httpmethod中的string一样，那么返回其对应的name，否则返回无效方法INVALID_METHOD
HttpMethod StringToHttpMethod(const std::string& m){
#define XX(num,name,string) \
    if(strcmp(#string,m.c_str())==0){ \
        return HttpMethod::name; \
    } 
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::HTTP_INVALID_METHOD;
}

// m 字符串方法枚举
HttpMethod CharsToHttpMethod(const char* m){
#define XX(num,name,string) \
    if(strncmp(#string,m,strlen(#string))==0){ \
        return HttpMethod::name; \
    } 
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::HTTP_INVALID_METHOD;
}

static const char* m_method_string[]={
    //存放的是XX中的最后一项stirng
#define XX(num,name,string) #string,  
    HTTP_METHOD_MAP(XX)
#undef XX
};
//这个也可以用switch
const char* HttpMethodToString(const HttpMethod& m){
    //将方法转化为uint32_t类型
    uint32_t idx=(uint32_t)m;
    //如果输入的数字比数组的大小还大，返回<unknow>
    if(idx>(sizeof(m_method_string) /sizeof(m_method_string[0]))){
        return "<unknow>";
    }
    return m_method_string[idx];
}
const char* HttpStatusToString(const HttpStatus& s){
    switch (s){
#define XX(code,name,msg) \
    case HttpStatus::name: \
        return #msg; 
        HTTP_STATUS_MAP(XX);
#undef XX
    default:
        return "<unknow>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string& lhs,const std::string& rhs) const{
    //C语言中判断字符串是否相等的函数，忽略大小写
    //小于0	s1 小于s2,true
    return strcasecmp(lhs.c_str(),rhs.c_str()) <0;
}

HttpRequest::HttpRequest(uint8_t version,bool close)
        :m_method(HttpMethod::GET)
        ,m_version(version)
        ,m_close(close)
        ,m_path("/"){
}
std::string HttpRequest::getHeader (const std::string& key,const std::string& def) const{
    auto it=m_headers.find(key);
    return it==m_headers.end()? def:it->second;
}
std::string HttpRequest::getParams (const std::string& key,const std::string& def){
    auto it=m_params.find(key);
    return it==m_params.end()? def:it->second;
}
std::string HttpRequest::getCookie (const std::string& key,const std::string& def){
    auto it=m_cookies.find(key);
    return it==m_cookies.end()? def:it->second;
}
void HttpRequest::setHeader(const std::string& key,const std::string& def) {
    m_headers[key]=def;
}
void HttpRequest::setParam(const std::string& key,const std::string& def) {
    m_params[key]=def;
}
void HttpRequest::setCookie(const std::string& key,const std::string& def) {
    m_cookies[key]=def;
}
void HttpRequest::delHeader(const std::string& key) {
    m_headers.erase(key);
}
void HttpRequest::delParam(const std::string& key) {
    m_params.erase(key);
}
void HttpRequest::delCookie(const std::string& key) {
    m_cookies.erase(key);
}
bool HttpRequest::hasHeader(const std::string& key,std::string* val){
    auto it=m_headers.find(key);
    if(it==m_headers.end()){
        return false;
    }
    if(val){
        *val=it->second;
    }
    return true;
}
bool HttpRequest::hasParam(const std::string& key,std::string* val){
    auto it=m_params.find(key);
    if(it==m_params.end()){
        return false;
    }
    if(val){
        *val=it->second;
    }
    return true;
}
bool HttpRequest::hasCookie(const std::string& key,std::string* val){
    auto it=m_cookies.find(key);
    if(it==m_cookies.end()){
        return false;
    }
    if(val){
        *val=it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const{
    //GET /uri HTTP/1.1
    //header::  Host: www.sylar.top
    //body
    //
    os<<HttpMethodToString(m_method)<<" "
      <<m_path
      <<(m_query.empty() ? "" :"?")
      <<m_query
      <<(m_fragment.empty() ? "" : "#")
      <<m_fragment
      <<" HTTP/"
      <<(uint32_t)(m_version >> 4)
      <<"."
      <<(uint32_t)(m_version& 0x0F)
      <<"\r\n";
    os<<"connection: "<<(m_close ? "close" : "keep-alive")<<"\r\n";
    for(auto& i :m_headers){
        if(strcasecmp(i.first.c_str(),"connection")==0){
            continue;
        }
        os<<i.first<<":"<<i.second<<"\r\n";
    }

    if(!m_body.empty()){
        os<<"content-lenth:"<<m_body.size()<<"\r\n\r\n"
            <<m_body;
    }else{
        os<<"\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const{
    std::stringstream ss;
    dump(ss);
    return  ss.str();
}


HttpResponse::HttpResponse(uint8_t version,bool close)
    :m_status(HttpStatus::OK)
    ,m_version(version)
    ,m_close(close){
}
std::string HttpResponse::getHeader (const std::string& key,const std::string& def) const{
    auto it=m_headers.find(key);
    return it==m_headers.end() ? def: it->second;
}
void HttpResponse::setHeader(const std::string& key,const std::string& val){
    m_headers[key]=val;
}
void HttpResponse::delHeader(const std::string& key){
    m_headers.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os) const{
    os<<"HTTP/"
      <<((uint32_t)(m_version>>4))
      <<"."
      <<((uint32_t)(m_version & 0x0F))
      <<" "
      <<(uint32_t)m_status
      <<" "
      <<(m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
      <<"\r\n";

    for(auto& i:m_headers){
        if(strcasecmp(i.first.c_str(),"connection")==0){
            continue;
        }
        os<<i.first<<": "<<i.second<<"\r\n";
    }
    os<<"connection: "<<(m_close ? "close" : "keep-alive")<<"\r\n";

    if(!m_body.empty()){
        os<<"content-length: "<<m_body.size()<<"\r\n\r\n"
            <<m_body;
    }else{
        os<<"\r\n";
    }
    return os;
}

std::string HttpResponse::toString() const{
    std::stringstream ss;
    dump(ss);
    return  ss.str();
}

std::ostream& operator<<(std::ostream& os,const HttpRequest& req){
    return req.dump(os);
}
std::ostream& operator<<(std::ostream& os,const HttpResponse& rsp){
    return rsp.dump(os);
}

}
}