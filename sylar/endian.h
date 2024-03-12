#ifndef _SYLAR_ENDIAN_H__
#define _SYLAR_ENDIAN_H__
 //字节序操作函数(大端/小端)
#define SYLAR_LITTLE_ENDIAN 1//本地字节序
#define SYLAR_BIG_ENDIAN 2   //网络字节序

#include<byteswap.h>
#include<stdint.h>
#include<type_traits>

namespace sylar{


//8字节类型的字节序转化
template<class T>
//判断T和uint64的大小是否一样，如果一样执行下面函数
typename std::enable_if<sizeof(T)==sizeof(uint64_t),T>::type
byteswap(T value){
    return (T)bswap_64((uint64_t)value);
}

//4字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint32_t),T>::type
byteswap(T value){
    return (T)bswap_32((uint32_t)value);
}

//2字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T)==sizeof(uint16_t),T>::type
byteswap(T value){
    return (T)bswap_16((uint16_t)value);
}
//BYTE_ORDER是当前机器的字节序
#if BYTE_ORDER==BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER==SYLAR_BIG_ENDIAN
/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template<class T>
T byteswapOnLittleEndian(T t){
    return t;
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template<class T>
T byteswapOnBigEndian(T t){
    return byteswap(t);
}

#else
//本台机器是小端机器，只执行下面这两个
template<class T>
T byteswapOnLittleEndian(T t){
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t){
    return t;
}

#endif
}



#endif