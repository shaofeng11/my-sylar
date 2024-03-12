#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__
#include<memory>
#include<string>
#include<iostream>
#include<stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<vector>
namespace sylar{

class ByteArray{
public:
    typedef std::shared_ptr<ByteArray>ptr;

    // ByteArray的存储节点
    struct Node{
        // 构造指定大小的内存块,s字节数
        Node(size_t s);
        Node();
        ~Node();

    // 内存块地址指针
        char* ptr;

        Node* next;
        // 内存块大小
        size_t size;
    };

    ByteArray(size_t base_size=4096);
    ~ByteArray();

    //write   固定长度
    //固定长度的有符号/无符号8位、16位、32位、64位整数
    void writeFint8(int8_t value);//1字节
    void writeFuint8(uint8_t value);
    //需考虑大端小端
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value); 
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);


    //不用考虑字节序
    //不固定长度的有符号/无符号32位、64位整数，数据压缩
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value); 
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    //float、double类型
    void writeFloat(float value);
    void writeDouble(double value);

    //字符串，包含字符串长度，长度范围支持16位、32位、64位
    //length: int16,data
    void writeString16(const std::string& value);
     //length: int32,data
    void writeString32(const std::string& value);
     //length: int64,data
    void writeString64(const std::string& value);
    //压缩的方式
    //length:varint,data
    void writeStringVint(const std::string& value);


    //字符串，不包含长度。
    //data
    void writeStringWithoutLength(const std::string& value);

    //read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    //length:int16 ,data
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    //内部操作
    void clear();

    void write(const void* buf,size_t size);
    void read(void* buf,size_t size);
    void read(void* buf,size_t size,size_t position) const;
    
    size_t getPosition() const { return m_position;}
    void setPosition(size_t v);

    bool writeToFile(const std::string& name)const;
    bool readFromFile(const std::string& name); 

    //返回内存块的大小
    size_t getBaseSize() const{return m_baseSize;}  
    //可读数据的大小  当前数据大小-当前位置
    size_t getReadSize() const{return m_size-m_position;}

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString() const;

    //从当前位置的所有数据全部放入buffers中
    uint64_t getReadBuffers(std::vector<iovec>&buffers,uint64_t len=~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>&buffers,uint64_t len,uint64_t position) const;

    //增加容量，不改变position
    uint64_t getWriteBuffers(std::vector<iovec>&buffers,uint64_t len);

    size_t getSize() const{return m_size; }

private:
    void addCapacity(size_t size);
    //已分配的内存-当前位置
    size_t getCapacity() const { return m_capacity-m_position;}
private:
    //内存块大小，每一个node的大小
    size_t m_baseSize;
    //当前操作位置
    size_t m_position;
    //内存块容量，节点数*base_size
    size_t m_capacity;
    //当前数据的大小，当前真实大小
    size_t m_size;
    // 字节序,默认大端
    int8_t m_endian;
    // 第一个内存块指针，存放数据
    Node* m_root;
    // 当前操作的内存块指针
    Node* m_cur;
};

}

#endif