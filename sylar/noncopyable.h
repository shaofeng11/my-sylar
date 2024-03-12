#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace sylar{
class Noncopyable{
public:
    Noncopyable()=default;
    ~Noncopyable()=default;
    //禁用拷贝构造
    Noncopyable(const Noncopyable&)=delete;
    //赋值
    Noncopyable& operator=(const Noncopyable&)=delete;
};

}
#endif