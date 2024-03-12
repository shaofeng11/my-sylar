#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include<string.h>
#include<assert.h>
#include"util.h"


//likely(x)表示x条件很可能为真，编译器应该优化为更好地支持这种情况。这在if语句或循环中表示条件为真的情况更频繁。
//unlikely(x)表示x条件不太可能为真，编译器应该优化为更好地支持这种情况。这在if语句或循环中表示条件为假的情况更频繁。

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define SYLAR_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define SYLAR_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif


//#x: 取x的内容    \n:换行
#define SYLAR_ASSERT(x) \
    if(SYLAR_LIKELY(!(x))) {       \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION:" #x \
                                        <<"\nbacktrace:\n" \
                                        <<sylar::BacktraceToString(100,2,"    ");\
        assert(x); \
    }

#define SYLAR_ASSERT2(x,w) \
    if(SYLAR_UNLIKELY(!(x))) {       \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION:" #x \
                                        <<"\n"<<w \
                                        <<"\nbacktrace:\n" \
                                        <<sylar::BacktraceToString(100,2,"    ");\
        assert(x); \
    }

#endif