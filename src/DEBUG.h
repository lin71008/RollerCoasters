#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifndef NDEBUG
    #define DEBUG_INFO(fmt, ...) \
    do{\
        fprintf(stderr, fmt, ##__VA_ARGS__);\
    }while(0)
#else
    #define DEBUG_INFO(fmt, ...) {}
#endif

#ifdef __cplusplus
}
#endif
#endif