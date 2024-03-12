#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include<memory>
#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/un.h>
#include<iostream>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<vector>
#include<map>
namespace sylar{

class IPAddress;

class Address {
public:
    typedef std::shared_ptr<Address>ptr;


       /**
     * @brief 通过host域名地址返回对应条件的所有Address
     * @param[out] result 保存满足条件的Address
     * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM(流式、报式) 等
     * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
     * @return 返回是否转换成功
     */
    static bool Lookup(std::vector<Address::ptr>&result,const std::string host,
                int family=AF_UNSPEC,int type=0,int protocol=0);
     /**
     * @brief 通过host地址返回对应条件的任意Address
     * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
     * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
     * @return 返回满足条件的任意Address,失败返回nullptr
     */

    //返回上面那个方法得到的数组的第一个元素
    static Address::ptr LookupAny(const std::string host,
                int family=AF_UNSPEC,int type=0,int protocol=0);

    //在所有ip地址中随机返回一个
    static std::shared_ptr<IPAddress> LookupAnyIpAddress(const std::string host,
                int family=AF_INET,int type=0,int protocol=0);


    /**
     * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
     * @param[out] result 保存本机所有地址
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::multimap<std::string
                                    ,std::pair<Address::ptr,uint32_t>>&result
                                    ,int family=AF_UNSPEC);
    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     * @param[out] result 保存指定网卡所有地址
     * @param[in] iface 网卡名称
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr,uint32_t>>&result
                                    ,const std::string& iface,
                                    int family=AF_UNSPEC);
    
    /**
     * @brief 通过sockaddr指针创建Address
     * @param[in] addr sockaddr指针
     * @param[in] addrlen sockaddr的长度
     * @return 返回和sockaddr相匹配的Address,失败返回nullptr  若sockaddr式ipv4，就返回ipv4的address
     */
    static Address::ptr Create(const sockaddr* addr,socklen_t addrlen);

    virtual ~Address(){}

    //返回协议簇
    int getFamily() const;

    virtual const sockaddr* getAddr() const=0;
    virtual  sockaddr* getAddr() =0;
    virtual socklen_t getAddrLen() const=0;

    //可读性输出地址
    virtual std::ostream& insert(std::ostream& os)const=0;

    //返回可读性字符串
    std::string toString() const;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

};

class IPAddress:public Address{
public:
    typedef std::shared_ptr<IPAddress>ptr;

    /**
     * @brief 通过域名,IP,服务器名创建IPAddress
     * @param[in] address 域名,IP,服务器名等.举例: www.sylar.top
     * @param[in] port 端口号
     * @return 调用成功返回IPAddress,失败返回nullptr
     */    
    static IPAddress::ptr Create(const char* address,uint16_t port=0);
    /**
     * @brief 获取该地址的广播地址
     * @param[in] prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len)=0;

      /**
     * @brief 获取该地址的网段
     * @param[in] prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len)=0;

     /**
     * @brief 获取子网掩码地址
     * @param[in] prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len)=0;

    virtual uint32_t getPort() const=0;
    virtual void setPort(uint16_t v)=0;//sin_port的类型为in_port_t，in_port_t的类型为uint16_t
};

class IPv4Address:public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address>ptr;

    //将文本型的地址转成address family
    static IPv4Address::ptr Create(const char* address,uint16_t port=0);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address=INADDR_ANY,uint16_t port=0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const;
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;;
    void setPort(uint16_t v) override;
private:
    sockaddr_in m_addr;
};

class IPv6Address:public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address>ptr;

    static IPv6Address::ptr Create(const char* address,uint16_t port=0);

    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16],uint16_t port=0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const;
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

class UnixAddress:public Address{
public:
    typedef std::shared_ptr<UnixAddress>ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::ostream& insert(std::ostream& os)const;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknowAddress:public Address{
public:
    typedef std::shared_ptr<UnknowAddress>ptr;
    UnknowAddress(int family);
    UnknowAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr()override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os,const Address& addr);
}

#endif