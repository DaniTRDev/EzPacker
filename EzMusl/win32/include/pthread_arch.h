#ifndef _EZMUSL_WIN32_PTHREAD_ARCH_H
#define _EZMUSL_WIN32_PTHREAD_ARCH_H

#include <stdint.h>

uintptr_t __ez_win32_get_tp(void);

static inline uintptr_t __get_tp(void)
{
    return __ez_win32_get_tp();
}

#define MC_PC gregs[16]

#endif
