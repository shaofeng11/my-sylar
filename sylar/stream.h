#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__
#include<memory>
#include"bytearray.h"

namespace sylar{

class Stream{
public:
    typedef std::shared_ptr<Stream>ptr;
    virtual ~Stream(){}


   virtual int read(void* buffer,size_t length)=0;
   virtual int read(sylar::ByteArray::ptr ba,size_t length)=0;
   virtual int readFixSize(void* buffer,size_t length);
   virtual int readFixSize(sylar::ByteArray::ptr ba,size_t length);
   virtual int write(const void* buffer,size_t length)=0;
   virtual int write(sylar::ByteArray::ptr ba,size_t length)=0;
   virtual int writeFixSize(const void* buffer,size_t length);
   virtual int writeFixSize(sylar::ByteArray::ptr ba,size_t length);
   virtual void close()=0;
};

}
#endif