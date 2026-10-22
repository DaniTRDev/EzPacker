#ifndef _EZPACKER_STDDEF_H
#define _EZPACKER_STDDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef unsigned long long size_t;

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__)
typedef long long ptrdiff_t;
typedef long long intptr_t;
typedef unsigned long long uintptr_t;
typedef unsigned short wchar_t;
#else
typedef long ptrdiff_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef unsigned int wchar_t;
#endif

typedef long double max_align_t;

#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_STDDEF_H
