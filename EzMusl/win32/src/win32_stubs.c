#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "pthread_impl.h"

/* Thread / cancellation stubs for single-process Win32 musl runtime */
weak long __syscall_cp_c(syscall_arg_t nr,
                         syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                         syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
    return (long)__ez_win32_syscall((long)nr, u, v, w, x, y, z);
}

long (__syscall_cp)(syscall_arg_t nr,
                    syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                    syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
    return (long)__ez_win32_syscall((long)nr, u, v, w, x, y, z);
}

long __syscall_cp_asm(volatile void *cancel, syscall_arg_t nr,
                      syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                      syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
    (void)cancel;
    return (long)__ez_win32_syscall((long)nr, u, v, w, x, y, z);
}

void __unmapself(void *base, size_t size)
{
    (void)base;
    (void)size;
    __ez_win32_syscall(SYS_exit, 0, 0, 0, 0, 0, 0);
}

int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...)
{
    (void)func; (void)stack; (void)flags; (void)arg;
    return -ENOSYS;
}

pid_t vfork(void)
{
    errno = ENOSYS;
    return -1;
}

void __restore_rt(void)
{
}
weak_alias(__restore_rt, __restore);

int __sigsetjmp(sigjmp_buf env, int savemask)
{
    (void)env;
    (void)savemask;
    return 0;
}

/* Note: setjmp/longjmp in musl have portable C stubs in src/setjmp/ that return 0;
   we can keep those or add Win64 register save/restore later if needed. */

/* Strong COFF archive entry points for musl hooks whose canonical definition
   uses weak_alias in a file shared with optional dummy hooks. */
extern void *__libc_malloc_impl(size_t n);
void *malloc(size_t n)
{
    return __libc_malloc_impl(n);
}

extern void __stdio_exit(void);
void __stdio_exit_needed(void)
{
    __stdio_exit();
}

#if defined(__GNUC__) || defined(__clang__)
__asm__(
    ".globl __init_array_start\n"
    ".set __init_array_start, 0\n"
    ".globl __init_array_end\n"
    ".set __init_array_end, 0\n"
    ".globl __fini_array_start\n"
    ".set __fini_array_start, 0\n"
    ".globl __fini_array_end\n"
    ".set __fini_array_end, 0\n"
    ".globl _DYNAMIC\n"
    ".set _DYNAMIC, 0\n"
);
#endif
