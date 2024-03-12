#include"http_session.h"
#include "http_parser.h"
namespace sylar{
namespace http{
HttpSession::HttpSession(Socket::ptr sock,bool owner)
    :SocketStream(sock,owner){
}

//一个字:绝
HttpRequest::ptr HttpSession::recvRequest(){
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size=HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size=50;

    //创建一个智能指针数组
    std::shared_ptr<char>buffer(
        new char[buff_size],[](char* ptr){
            delete[] ptr;
         });
    
    //拿到裸指针数组,data指向数组的首地址
    char* data=buffer.get();
    int offset=0;//地址偏移量
    do{
        int len=read(data+offset,buff_size-offset);
        if(len<=0){
            close();
            return nullptr;
        }
        len+=offset;
        size_t nparser=parser->excute(data,len);
        if(parser->hasError()){
            close();
            return nullptr;
        }
        offset=len-nparser;
        if(offset==(int)buff_size){
            close();
            return nullptr;
        }
        if(parser->isFinish()){
            break;
        }
    }while(true);
    int64_t length=parser->getContentLength();
    if(length>0){
        std::string body;
        body.resize(length);

        int len=0;
        if(length>=offset){
            memcpy(&body[0],data,offset);
            len=offset;
        }else{
            memcpy(&body[0],data,length);
            len=length; 
        }
        length-=offset;//比如header中的长度为12，实际body中只有10，那么length=2
        if(length>0){
            if(readFixSize(&body[len],length)<=0){
                close();
                return nullptr;
            }
        }
    parser->getData()->setBody(body);
    }
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp){
    std::stringstream ss;
    ss<<*rsp;
    std::string data=ss.str();
    return writeFixSize(ss.str().c_str(),data.size());
}
}
}