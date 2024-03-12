#include"address.h"
#include"log.h"
#include<sstream>
#include<stddef.h>
#include"endian.h"
#include<ifaddrs.h>
namespace sylar{

static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

template<class T>
static T CreateMask(uint32_t bits){
    return (1 << (sizeof(T) * 8 -bits))-1;
}

template<class T>
static uint32_t CountByte(T value){
    uint32_t result=0;
    for(;value;++result){
        value&=value-1;
    }
    return result;
}

bool Address::Lookup(std::vector<Address::ptr>&result,const std::string host,
                int family,int type,int protocol){
    //地址结构体  初始化
    addrinfo hints,*results,*next;
    hints.ai_flags=0;
    hints.ai_family=family;
    hints.ai_protocol=protocol;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_addrlen=0;
    hints.ai_canonname=NULL;
    hints.ai_addr=NULL;
    hints.ai_next=NULL;

    std::string node;
    const char* service=NULL;

    //检查 ipv6address serivce
    if(!host.empty()&&host[0]=='['){
        //在host.c_str()+1中前host.size()-1中寻找]的位置,返回值是一个指针
        const char* endipv6=(const char*)memchr(host.c_str()+1,']',host.size()-1);
        if(endipv6){
            //TODO check out of range
            if(*(endipv6+1)==':'){
                service=endipv6+2;
            }
            //从第一个位置开始截取endipv6-host.c_str()-1个字符
            node=host.substr(1,endipv6-host.c_str()-1);
        }
    }
    if(node.empty()){
        service=(const char*)memchr(host.c_str(),':',host.size());
        if(service){
            if(!memchr(service+1,':',host.c_str()+host.size()-service-1)){
                node=host.substr(0,service-host.c_str());
                ++service;
            }
        }
    }
    if(node.empty()){
        node=host;
    }  
    //getaddrinfo对www.baidu.com进行解析
    //results为一个指向存储结果的 struct addrinfo 结构体列表  addrinfo里存放的有
// struct addrinfo
// {
//      int ai_flags; /* customize behavior */
//      int ai_family; /* address family */
//      int ai_socktype; /* socket type */
//      int ai_protocol; /* protocol */
//      socklen_t ai_addrlen; /* length in bytes of address */
//      struct sockaddr *ai_addr; /* address */
//      char *ai_canonname; /* canonical name of host */
//      struct addrinfo *ai_next; /* next in list */
 
    //service服务名可以是十进制的端口号("8080")字符串，也可以是已定义的服务名称，如"ftp"、"http"等会被翻译成端口号,也可以直接传端口号
    int error=getaddrinfo(node.c_str(),service,&hints,&results);
    if(error){ 
        SYLAR_LOG_ERROR(g_logger)<<"Address:Lookup getaddress("<<host<<","
                <<family<<","<<type<<") err="<<error<<"errstr="
                <<strerror(errno);
            return false;
    }
    next=results;
    //results是一个链表，可以存放多个地址，将解出来的地址全部放入vector容器中
    while(next){
        //Create 将sockaddr转化为ipv4，v6
        result.push_back(Create(next->ai_addr,next->ai_addrlen));
        next=next->ai_next;
    }
    freeaddrinfo(results);
    return true; 
}

Address::ptr Address::LookupAny(const std::string host,
                int family,int type,int protocol){
    std::vector<Address::ptr>result;
    if(Lookup(result,host,family,type,protocol)){
        //返回result[0]??
        return result[0];
    }
    return nullptr;
}
std::shared_ptr<IPAddress> Address::LookupAnyIpAddress(const std::string host,
                int family,int type,int protocol){
    std::vector<Address::ptr>result;
    if(Lookup(result,host,family,type,protocol)){
       for(auto& i:result){
        IPAddress::ptr v=std::dynamic_pointer_cast<IPAddress>(i);
        //如果v不为空，说明是IP地址，直接返回
        if(v){
            return v;
        }
       }
    }
    return nullptr;
}

bool  Address::GetInterfaceAddresses(std::multimap<std::string
                                    ,std::pair<Address::ptr,uint32_t>>&result
                                    ,int family){
    struct ifaddrs *next,*results;
    //获取本地网络接口的信息。在路由器上可以用这个接口来获取wan/lan等接口当前的ip地址，广播地址等信息。
    if(getifaddrs(&results)!=0){
        SYLAR_LOG_ERROR(g_logger)<<"Address::GetInterfaceAddresses getifaddrs"
                    <<"err="<<errno<<"errstr="<<strerror(errno);
        return false;
    }
    //results为一个链表，链表上的每个节点都是一个struct ifaddrs结构，getifaddrs()返回链表第一个元素的指针
    try{
        //next不为空，一直循环
        for(next=results;next;next=next->ifa_next){
            Address::ptr addr;
            //uint32_t prefix_length=~0u;//无穷大
            uint32_t prefix_length=0;
            //如果family！=AF_UNSPEC
            if(family!=AF_UNSPEC&&family!=next->ifa_addr->sa_family){
                continue;
            }
            switch(next->ifa_addr->sa_family){
                case AF_INET:
                    {
                        addr=Create(next->ifa_addr,sizeof(sockaddr_in));
                        //mask地址
                        uint32_t netmask=((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        //返回子网掩码中1的个数
                        prefix_length=CountByte<uint32_t>(netmask);//编译器自动推导，尖括号中写不写都行
                    }
                    break;
                case AF_INET6:
                    {
                        addr=Create(next->ifa_addr,sizeof(sockaddr_in6));
                        in6_addr& netmask=((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        for(int i=0;i<16;++i){
                            prefix_length+=CountByte(netmask.s6_addr[i]);  
                        }
                    }
                    break;
                default:
                    break;
            }
        if(addr){
            result.insert(std::make_pair(next->ifa_name,
                        std::make_pair(addr,prefix_length)));
        }
        }
    }
    catch(...){
        SYLAR_LOG_ERROR(g_logger)<<"Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return true;
}
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr,uint32_t>>&result
                                    ,const std::string& iface,int family){
    //任一网卡
    if(iface.empty()||iface=="*"){
        if(family==AF_INET||family==AF_UNSPEC){
            //返回0.0.0.0，子网掩码位数也为0
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()),0u));
        }
        if(family==AF_INET6||family==AF_UNSPEC){
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()),0u));
        }
        return true;
    }

    std::multimap<std::string,std::pair<Address::ptr,uint32_t>>results;
    if(!GetInterfaceAddresses(results,family)){
        return false;
    }
    //找到所有key为iface的键值对，its的first是第一个键值对，second是最后一个键值对
    auto its=results.equal_range(iface);
    for(;its.first!=its.second;++its.first){
        result.push_back(its.first->second);
    }
    return true;
}

Address::ptr Address::Create(const sockaddr* addr,socklen_t addrlen){
    if(addr==nullptr){
        return nullptr;
    }
    Address::ptr result;
    switch(addr->sa_family){
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknowAddress(*addr));
            break;
    }
    return result;
}

int Address::getFamily() const{
    return getAddr()->sa_family;
}
std::string Address::toString() const{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}
bool Address::operator<(const Address& rhs) const{
    //取两个地址长度的较小值
    socklen_t minlen=std::min(getAddrLen(),rhs.getAddrLen());

    //比较前minlen个
    int result=memcmp(getAddr(),rhs.getAddr(),minlen);
    if(result<0){//表明buf1<buf2
        return true;
    }else if(result>0){//表明buf1>buf2
        return false;
    //前minlen个相等并且buf1的长度小于buf2
    }else if(getAddrLen()<rhs.getAddrLen()){
        return true;
    }
    return false;
}
bool Address::operator==(const Address& rhs) const{
    return getAddrLen()==rhs.getAddrLen()
    //其功能是把存储区 str1 和存储区 str2 的前 n 个字节进行比较。该函数是按字节比较的
            &&memcmp(getAddr(),rhs.getAddr(),getAddrLen())==0;
}
bool Address::operator!=(const Address& rhs) const{
    return !(*this==rhs);
}

IPAddress::ptr IPAddress::Create(const char* address,uint16_t port){
    addrinfo hints,*results;
    memset(&hints,0,sizeof(addrinfo));

    //hints.ai_flags=AI_NUMERICHOST;
    hints.ai_family=AF_UNSPEC;
    //如果 ai_flags 中设置了AI_NUMERICHOST 标志，那么该参数(第一个参数)只能是数字化的地址字符串，不能是域名，
    //如果此参数设置为NULL，那么返回的socket地址中的端口号不会被设置。
    //Returns: 0 if OK, nonzero error code on error
    int error=getaddrinfo(address,NULL,&hints,&results);
    if(error){
        SYLAR_LOG_ERROR(g_logger)<<"IPAddress::Create("<<address<<","
                                <<"port"<<")"<<"errno="<<error
                                <<"errstr="<<strerror(errno);
        return nullptr;
    }
    try{
        IPAddress::ptr result=std::dynamic_pointer_cast<IPAddress>(  
            //addrinfo的ai_addr是sockaddr类型
                Address::Create(results->ai_addr,results->ai_addrlen));
        if(result){
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    }catch(...){
        freeaddrinfo(results);
        return nullptr;
    }
}

IPv4Address::ptr IPv4Address::Create(const char* address,uint16_t port){
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port=byteswapOnLittleEndian(port);
    int result=inet_pton(AF_INET,address,&rt->m_addr.sin_addr.s_addr);//文本转网络的函数
    if(result<=0){
        SYLAR_LOG_ERROR(g_logger)<<"IPv4Address::Create("<<address<<","
                                <<port<<")"<<result<<" errno="<<errno
                                <<" errstr="<<strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr=address;
}
IPv4Address::IPv4Address(uint32_t address,uint16_t port){
    //对结构体初始化
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin_family=AF_INET;//ipv4的family

    m_addr.sin_port=byteswapOnLittleEndian(port);//把port转成网络字节序赋值给m_addr.sin_port
    m_addr.sin_addr.s_addr=byteswapOnLittleEndian(address);//把地址转成网络字节序赋值给m_addr.sin_port
}

const sockaddr* IPv4Address::getAddr() const{
    return (sockaddr*)&m_addr;
}

sockaddr* IPv4Address::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t IPv4Address::getAddrLen() const{
    return sizeof(m_addr);
}
std::ostream& IPv4Address::insert(std::ostream& os)const{
    //将int型的地址转成ipv4类型的
    uint32_t addr=byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os<<((addr>>24) & 0xff)<<"."//0x表示16进制表示255
      <<((addr>>16)&0xff)<<"."
      <<((addr>>8)&0xff)<<"."
      <<(addr&0xff);
    os<<":"<<byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}
//广播地址与网络地址都需要用到子网掩码
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len){
    if(prefix_len>32){
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |=byteswapOnLittleEndian(
                CreateMask<uint32_t>(prefix_len));//将地址位或上其子网掩码
    return IPv4Address::ptr(new IPv4Address(baddr));//返回改过之后的地址
}
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len){
    if(prefix_len>32){
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &=byteswapOnLittleEndian(
                CreateMask<uint32_t>(prefix_len));//将地址位与上其子网掩码
    return IPv4Address::ptr(new IPv4Address(baddr));//返回改过之后的地址
}


IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len){
    sockaddr_in subnet;
    memset(&subnet,0,sizeof(subnet));
    subnet.sin_family=AF_INET;
    subnet.sin_addr.s_addr=~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));//对掩码取反
    return IPv4Address::ptr(new IPv4Address(subnet));
}
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port=byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char* address,uint16_t port){
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port=byteswapOnLittleEndian(port);
    int result=inet_pton(AF_INET6,address,&rt->m_addr.sin6_addr.s6_addr);
    if(result<=0){
        SYLAR_LOG_ERROR(g_logger)<<"IPv6Address::Create("<<address<<","
                                <<"port"<<")"<<result<<"errno="<<errno
                                <<"errstr="<<strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address(){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin6_family=AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address){
    m_addr=address;
}
IPv6Address::IPv6Address(const uint8_t address[16],uint16_t port){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin6_family=AF_INET6;
    m_addr.sin6_port=byteswapOnLittleEndian(port);//转成网络字节序
    memcpy(&m_addr.sin6_addr,address,16);
}

const sockaddr* IPv6Address::getAddr() const{
    return (sockaddr*)&m_addr;
}
sockaddr* IPv6Address::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t IPv6Address::getAddrLen() const{
    return sizeof(m_addr);
}
std::ostream& IPv6Address::insert(std::ostream& os)const{
    os<<"[";
    uint16_t* addr=(uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros=false;
    for(size_t i=0;i<8;++i){
        if(addr[i]==0&&!used_zeros){
            continue;
        }
        if(i&&addr[i-1]==0&&!used_zeros){
            os<<":";
            used_zeros=true;
        }
        if(i){
            os<<":";
        }
        //16进制
        os<<std::hex<<(int)byteswapOnLittleEndian(addr[i])<<std::dec;
    }
    if(!used_zeros&&addr[7]==0){
        os<<"::";
    }
    os<<"]:"<<byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len){
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len/8]|=
        CreateMask<uint8_t>(prefix_len %8);
    for(int i=prefix_len /8 +1;i<16;++i){
        baddr.sin6_addr.s6_addr[i]=0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len){
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len/8]&=
        CreateMask<uint8_t>(prefix_len %8);
    // for(int i=prefix_len /8 +1;i<16;++i){
    //     baddr.sin6_addr.s6_addr[i]=0xff;
    // }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len){
    sockaddr_in6 subnet;
    memset(&subnet,0,sizeof(subnet));
    subnet.sin6_family=AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8]=
        ~CreateMask<uint8_t>(prefix_len %8);
    for(uint32_t i=0;i<prefix_len /8;++i){
        subnet.sin6_addr.s6_addr[i]=0xFF;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port=byteswapOnLittleEndian(v);
}

//固定写法，记住
static const size_t MAX_PATH_LEN=sizeof((sockaddr_un*)0)->sun_path-1;
UnixAddress::UnixAddress(){
    //地址结构清零
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    //offsetof 本质上是 linux 内核的一个宏函数，其作用是获取结构体中某个成员相对于结构体起始地址的偏移量。
    m_length=offsetof(sockaddr_un,sun_path)+MAX_PATH_LEN;
}

//sun_path应存放文件系统的路径名。
UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    m_length=path.size()+1;

    if(!path.empty()&&path[0]=='\0'){
        --m_length;
    }
    if(m_length>sizeof(m_addr.sun_path)){
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path,path.c_str(),m_length);
    m_length+=offsetof(sockaddr_un,sun_path);
}

const sockaddr*  UnixAddress::getAddr() const{
    return (sockaddr*)&m_addr;
}
sockaddr* UnixAddress::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t  UnixAddress::getAddrLen() const{
    return m_length;
}
void UnixAddress::setAddrLen(uint32_t v){
    m_length=v;
}
std::ostream&  UnixAddress::insert(std::ostream& os)const{
    if(m_length > offsetof(sockaddr_un,sun_path)
            &&m_addr.sun_path[0]=='\0'){
        return os<<"\\0"<<std::string(m_addr.sun_path+1,
                m_length-offsetof(sockaddr_un,sun_path)-1);
    }
    return os<<m_addr.sun_path;
}
UnknowAddress::UnknowAddress(int family){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sa_family=family;
}
UnknowAddress::UnknowAddress(const sockaddr& addr){
    m_addr=addr;
}
const sockaddr* UnknowAddress::getAddr() const{
    return &m_addr;
}
sockaddr* UnknowAddress::getAddr(){
    return (sockaddr*)&m_addr;
}
socklen_t UnknowAddress::getAddrLen() const{
   return sizeof(m_addr);
}
std::ostream& UnknowAddress::insert(std::ostream& os)const{
    os<<"[UnknowAddress family="<<m_addr.sa_family<<"]";
    return os;
}

std::ostream& operator<<(std::ostream& os,const Address& addr){
    return addr.insert(os);
}
}