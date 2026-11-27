#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "syscall.h"

#define MEM_COMMIT   0x00001000
#define MEM_RESERVE  0x00002000
#define MEM_RELEASE  0x00008000

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40

typedef struct {
    void *BaseAddress;
    void *AllocationBase;
    unsigned long AllocationProtect;
    unsigned short PartitionId;
    size_t RegionSize;
    unsigned long State;
    unsigned long Protect;
    unsigned long Type;
} EZ_MEMORY_BASIC_INFORMATION;

__declspec(dllimport) void *__stdcall VirtualAlloc(void *lpAddress, size_t dwSize, unsigned long flAllocationType, unsigned long flProtect);
__declspec(dllimport) int __stdcall VirtualFree(void *lpAddress, size_t dwSize, unsigned long dwFreeType);
__declspec(dllimport) int __stdcall VirtualProtect(void *lpAddress, size_t dwSize, unsigned long flNewProtect, unsigned long *lpflOldProtect);
__declspec(dllimport) size_t __stdcall VirtualQuery(const void *lpAddress, EZ_MEMORY_BASIC_INFORMATION *lpBuffer, size_t dwLength);

void __vm_wait(void) {}

static unsigned long prot_to_win32(int prot)
{
    if (prot == PROT_NONE)
        return PAGE_NOACCESS;
    if (prot & PROT_EXEC)
        return (prot & PROT_WRITE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    if (prot & PROT_WRITE)
        return PAGE_READWRITE;
    return PAGE_READONLY;
}

void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    (void)fd;
    (void)off;

    if (len == 0 || len >= PTRDIFF_MAX) {
        errno = EINVAL;
        return MAP_FAILED;
    }

    if (!(flags & MAP_ANONYMOUS)) {
        errno = ENOSYS;
        return MAP_FAILED;
    }

    if (flags & MAP_FIXED) {
        __vm_wait();
    }

    /* Always allocate PAGE_READWRITE for anonymous allocations so mallocng can
       write metadata even if PROT_NONE was requested first and mprotect'd later,
       or use PAGE_NOACCESS when PROT_NONE is explicitly requested. */
    unsigned long win_prot = (prot == PROT_NONE) ? PAGE_NOACCESS : PAGE_READWRITE;
    void *ptr = VirtualAlloc(start, len, MEM_COMMIT | MEM_RESERVE, win_prot);
    if (!ptr && start != NULL && !(flags & MAP_FIXED)) {
        ptr = VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, win_prot);
    }
    if (!ptr) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    return ptr;
}
weak_alias(__mmap, mmap);

int __munmap(void *start, size_t len)
{
    (void)len;
    if (!start) {
        errno = EINVAL;
        return -1;
    }
    __vm_wait();
    if (!VirtualFree(start, 0, MEM_RELEASE)) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}
weak_alias(__munmap, munmap);

void *__mremap(void *old_addr, size_t old_len, size_t new_len, int flags, ...)
{
    if (new_len >= PTRDIFF_MAX || new_len == 0) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    if (!(flags & MREMAP_MAYMOVE)) {
        if (new_len <= old_len)
            return old_addr;
        errno = ENOMEM;
        return MAP_FAILED;
    }

    void *new_addr = VirtualAlloc(NULL, new_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!new_addr) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    if (old_addr && old_len > 0) {
        size_t copy_len = old_len < new_len ? old_len : new_len;
        memcpy(new_addr, old_addr, copy_len);
        VirtualFree(old_addr, 0, MEM_RELEASE);
    }
    return new_addr;
}
weak_alias(__mremap, mremap);

int __mprotect(void *addr, size_t len, int prot)
{
    unsigned long old_prot = 0;
    if (!VirtualProtect(addr, len, prot_to_win32(prot), &old_prot)) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}
weak_alias(__mprotect, mprotect);

int __madvise(void *addr, size_t len, int advice)
{
    (void)addr;
    (void)len;
    (void)advice;
    return 0;
}
weak_alias(__madvise, madvise);
