#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__
#include<unistd.h>
#include <stdint.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <fcntl.h>
 #include <sys/ioctl.h>
namespace sylar{
    bool is_hook_enable();
    void set_hook_enable(bool flag);


}

extern "C"{

//定义一个函数签名
//sleep相关
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;


//socket相关
//socket
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;
//connect
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen);
extern connect_fun connect_f;
//accept
typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

//read相关

//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;
//readv
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;
//recv
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;
//recvfrom
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;
//recvmsg
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

//write相关
//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;
//writev
typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

//send
typedef int (*send_fun)(int s, const void *msg, size_t len, int flags);
extern send_fun send_f;
//sendto
typedef int (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_fun sendto_f;
//sendmsg
typedef int (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

//close
typedef int (*close_fun)(int fd);
extern close_fun close_f;

//socket操作相关

//fcntl
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
extern fcntl_fun fcntl_f;

//ioctl
typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

//getsockopt
typedef int (*getsockopt_fun)(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

//setsockopt
typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd,const struct sockaddr* addr,socklen_t addrlen, uint64_t timeout_ms);
 }   


#endif