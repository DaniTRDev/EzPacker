#ifndef _EZLIBSTDC_OS_SUPPORT_H
#define _EZLIBSTDC_OS_SUPPORT_H

#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__)
#define EZ_OS_WINDOWS 1
typedef void *EZ_HANDLE;
#define EZ_STD_OUTPUT_HANDLE ((unsigned long)-11)
#define EZ_STD_ERROR_HANDLE  ((unsigned long)-12)

__declspec(dllimport) void __stdcall ExitProcess(unsigned int uExitCode);
__declspec(dllimport) EZ_HANDLE __stdcall GetStdHandle(unsigned long nStdHandle);
__declspec(dllimport) int __stdcall WriteFile(EZ_HANDLE hFile, const void *lpBuffer, unsigned long nNumberOfBytesToWrite, unsigned long *lpNumberOfBytesWritten, void *lpOverlapped);
__declspec(dllimport) EZ_HANDLE __stdcall GetProcessHeap(void);
__declspec(dllimport) void *__stdcall HeapAlloc(EZ_HANDLE hHeap, unsigned long dwFlags, size_t dwBytes);
__declspec(dllimport) void *__stdcall HeapReAlloc(EZ_HANDLE hHeap, unsigned long dwFlags, void *lpMem, size_t dwBytes);
__declspec(dllimport) int __stdcall HeapFree(EZ_HANDLE hHeap, unsigned long dwFlags, void *lpMem);

#else
#define EZ_OS_LINUX 1
#include <sys/syscall.h>
#include <unistd.h>
#endif

_Noreturn static inline void ez_os_exit(int code) {
#if defined(EZ_OS_WINDOWS)
    ExitProcess((unsigned int)code);
    for (;;) {}
#else
    syscall(SYS_exit, code);
    for (;;) {}
#endif
}

static inline long ez_os_write_stderr(const void *buf, size_t len) {
#if defined(EZ_OS_WINDOWS)
    EZ_HANDLE h = GetStdHandle(EZ_STD_ERROR_HANDLE);
    unsigned long written = 0;
    WriteFile(h, buf, (unsigned long)len, &written, NULL);
    return (long)written;
#else
    return (long)syscall(SYS_write, 2, buf, len);
#endif
}

static inline long ez_os_write_stdout(const void *buf, size_t len) {
#if defined(EZ_OS_WINDOWS)
    EZ_HANDLE h = GetStdHandle(EZ_STD_OUTPUT_HANDLE);
    unsigned long written = 0;
    WriteFile(h, buf, (unsigned long)len, &written, NULL);
    return (long)written;
#else
    return (long)syscall(SYS_write, 1, buf, len);
#endif
}

static inline void *ez_os_alloc(size_t size) {
#if defined(EZ_OS_WINDOWS)
    return HeapAlloc(GetProcessHeap(), 0, size ? size : 1);
#else
    void *p = (void *)syscall(SYS_mmap, NULL, size ? size : 4096, 0x3, 0x22, -1, 0);
    return (p == (void *)-1) ? NULL : p;
#endif
}

static inline void *ez_os_realloc(void *ptr, size_t new_size) {
#if defined(EZ_OS_WINDOWS)
    if (!ptr) return HeapAlloc(GetProcessHeap(), 0, new_size);
    return HeapReAlloc(GetProcessHeap(), 0, ptr, new_size);
#else
    if (!ptr) return ez_os_alloc(new_size);
    return ez_os_alloc(new_size);
#endif
}

static inline void ez_os_free(void *ptr) {
    if (!ptr) return;
#if defined(EZ_OS_WINDOWS)
    HeapFree(GetProcessHeap(), 0, ptr);
#else
#endif
}

#endif // _EZLIBSTDC_OS_SUPPORT_H
