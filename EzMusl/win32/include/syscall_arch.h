#ifndef _EZMUSL_WIN32_SYSCALL_ARCH_H
#define _EZMUSL_WIN32_SYSCALL_ARCH_H

#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

/* Override musl's default `long` syscall_arg_t because Windows x64 is LLP64 (long is 32-bit). */
typedef long long syscall_arg_t;
#define __scc(X) ((long long)(X))

long long __ez_win32_syscall(long n,
                             long long a1, long long a2, long long a3,
                             long long a4, long long a5, long long a6);

static __inline long long __syscall0(long n)
{
    return __ez_win32_syscall(n, 0, 0, 0, 0, 0, 0);
}

static __inline long long __syscall1(long n, long long a1)
{
    return __ez_win32_syscall(n, a1, 0, 0, 0, 0, 0);
}

static __inline long long __syscall2(long n, long long a1, long long a2)
{
    return __ez_win32_syscall(n, a1, a2, 0, 0, 0, 0);
}

static __inline long long __syscall3(long n, long long a1, long long a2, long long a3)
{
    return __ez_win32_syscall(n, a1, a2, a3, 0, 0, 0);
}

static __inline long long __syscall4(long n, long long a1, long long a2, long long a3, long long a4)
{
    return __ez_win32_syscall(n, a1, a2, a3, a4, 0, 0);
}

static __inline long long __syscall5(long n, long long a1, long long a2, long long a3, long long a4, long long a5)
{
    return __ez_win32_syscall(n, a1, a2, a3, a4, a5, 0);
}

static __inline long long __syscall6(long n, long long a1, long long a2, long long a3, long long a4, long long a5, long long a6)
{
    return __ez_win32_syscall(n, a1, a2, a3, a4, a5, a6);
}

#define IPC_64 0

#endif
