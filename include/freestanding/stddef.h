#ifndef _EZPACKER_STDDEF_H
#define _EZPACKER_STDDEF_H

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef unsigned long long size_t;
typedef long long          ptrdiff_t;
typedef unsigned int       wchar_t;
typedef long double        max_align_t;

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* _EZPACKER_STDDEF_H */

