#include "../sylar/http/http.h"
#include "../sylar/log.h"

// static const char* m_method_string[]={
//     //存放的是XX中的最后一项stirng
// #define XX(num,name,string) #string  
//     HTTP_METHOD_MAP(XX)
// #undef XX
// };
void test_request(){
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
    req->setHeader("host","www.sylar.top");
    req->setBody("hello sylar");

    req->dump(std::cout)<<std::endl;
    //req->toString();
}

void test_response(){
    sylar::http::HttpResponse::ptr res(new sylar::http::HttpResponse);
    res->setHeader("X-X","sylar");
    res->setBody("hello sylar");

    res->setStatus((sylar::http::HttpStatus)400);
    res->setClose(false);
    res->dump(std::cout)<<std::endl;
}

int main(int argc,char** argv){
    test_request();
    test_response();
    // for(auto i:m_method_string){
    //     std::cout<<i<<std::endl;
    // }
    return 0;
}