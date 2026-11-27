#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include "syscall.h"
#include "kstat.h"

/* Minimal Win32 kernel32.dll imports without pulling Windows SDK headers under -nostdinc */
typedef void *EZ_HANDLE;
typedef struct { unsigned long dwLowDateTime; unsigned long dwHighDateTime; } EZ_FILETIME;
typedef struct {
    unsigned long dwFileAttributes;
    EZ_FILETIME ftCreationTime;
    EZ_FILETIME ftLastAccessTime;
    EZ_FILETIME ftLastWriteTime;
    unsigned long dwVolumeSerialNumber;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
    unsigned long nNumberOfLinks;
    unsigned long nFileIndexHigh;
    unsigned long nFileIndexLow;
} EZ_BY_HANDLE_FILE_INFORMATION;

typedef struct {
    unsigned long dwFileAttributes;
    EZ_FILETIME ftCreationTime;
    EZ_FILETIME ftLastAccessTime;
    EZ_FILETIME ftLastWriteTime;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
} EZ_WIN32_FILE_ATTRIBUTE_DATA;

#define EZ_INVALID_HANDLE_VALUE ((EZ_HANDLE)(long long)-1)
#define EZ_STD_INPUT_HANDLE  ((unsigned long)-10)
#define EZ_STD_OUTPUT_HANDLE ((unsigned long)-11)
#define EZ_STD_ERROR_HANDLE  ((unsigned long)-12)

#define EZ_GENERIC_READ     0x80000000UL
#define EZ_GENERIC_WRITE    0x40000000UL
#define EZ_FILE_SHARE_READ  0x00000001UL
#define EZ_FILE_SHARE_WRITE 0x00000002UL
#define EZ_CREATE_NEW       1UL
#define EZ_CREATE_ALWAYS    2UL
#define EZ_OPEN_EXISTING    3UL
#define EZ_OPEN_ALWAYS      4UL
#define EZ_TRUNCATE_EXISTING 5UL
#define EZ_FILE_ATTRIBUTE_NORMAL    0x00000080UL
#define EZ_FILE_ATTRIBUTE_DIRECTORY 0x00000010UL
#define EZ_FILE_TYPE_CHAR   0x0002UL
#define EZ_FILE_TYPE_PIPE   0x0003UL
#define EZ_ERROR_BROKEN_PIPE 109UL
#define EZ_ERROR_HANDLE_EOF  38UL
#define EZ_ERROR_FILE_NOT_FOUND 2UL
#define EZ_ERROR_PATH_NOT_FOUND 3UL
#define EZ_ERROR_ACCESS_DENIED  5UL
#define EZ_ERROR_FILE_EXISTS    80UL
#define EZ_ERROR_ALREADY_EXISTS 183UL

__declspec(dllimport) void __stdcall ExitProcess(unsigned int uExitCode);
__declspec(dllimport) EZ_HANDLE __stdcall GetStdHandle(unsigned long nStdHandle);
__declspec(dllimport) int __stdcall ReadFile(EZ_HANDLE hFile, void *lpBuffer, unsigned long nNumberOfBytesToRead, unsigned long *lpNumberOfBytesRead, void *lpOverlapped);
__declspec(dllimport) int __stdcall WriteFile(EZ_HANDLE hFile, const void *lpBuffer, unsigned long nNumberOfBytesToWrite, unsigned long *lpNumberOfBytesWritten, void *lpOverlapped);
__declspec(dllimport) EZ_HANDLE __stdcall CreateFileA(const char *lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode, void *lpSecurityAttributes, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes, EZ_HANDLE hTemplateFile);
__declspec(dllimport) int __stdcall CloseHandle(EZ_HANDLE hObject);
__declspec(dllimport) int __stdcall SetFilePointerEx(EZ_HANDLE hFile, long long liDistanceToMove, long long *lpNewFilePointer, unsigned long dwMoveMethod);
__declspec(dllimport) unsigned long __stdcall GetFileType(EZ_HANDLE hFile);
__declspec(dllimport) int __stdcall GetConsoleMode(EZ_HANDLE hConsoleHandle, unsigned long *lpMode);
__declspec(dllimport) int __stdcall GetFileInformationByHandle(EZ_HANDLE hFile, EZ_BY_HANDLE_FILE_INFORMATION *lpFileInformation);
__declspec(dllimport) int __stdcall GetFileAttributesExA(const char *lpFileName, int fInfoLevelId, void *lpFileInformation);
__declspec(dllimport) int __stdcall DeleteFileA(const char *lpFileName);
__declspec(dllimport) int __stdcall MoveFileExA(const char *lpExistingFileName, const char *lpNewFileName, unsigned long dwFlags);
__declspec(dllimport) int __stdcall CreateDirectoryA(const char *lpPathName, void *lpSecurityAttributes);
__declspec(dllimport) int __stdcall RemoveDirectoryA(const char *lpPathName);
__declspec(dllimport) unsigned long __stdcall GetCurrentDirectoryA(unsigned long nBufferLength, char *lpBuffer);
__declspec(dllimport) int __stdcall SetCurrentDirectoryA(const char *lpPathName);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);
__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void __stdcall GetSystemTimeAsFileTime(EZ_FILETIME *lpSystemTimeAsFileTime);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *lpPerformanceCount);
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *lpFrequency);
__declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) int __stdcall DuplicateHandle(EZ_HANDLE hSourceProcessHandle, EZ_HANDLE hSourceHandle, EZ_HANDLE hTargetProcessHandle, EZ_HANDLE *lpTargetHandle, unsigned long dwDesiredAccess, int bInheritHandle, unsigned long dwOptions);
__declspec(dllimport) EZ_HANDLE __stdcall GetCurrentProcess(void);

#define EZ_MAX_FDS 1024

typedef struct {
    EZ_HANDLE handle;
    int open;
    int flags;
    int is_std;
} ez_fd_entry;

static ez_fd_entry g_fds[EZ_MAX_FDS];
static int g_fds_init = 0;

static void init_fds(void)
{
    if (g_fds_init) return;
    g_fds_init = 1;
    g_fds[0].handle = GetStdHandle(EZ_STD_INPUT_HANDLE);
    g_fds[0].open = 1; g_fds[0].flags = O_RDONLY; g_fds[0].is_std = 1;
    g_fds[1].handle = GetStdHandle(EZ_STD_OUTPUT_HANDLE);
    g_fds[1].open = 1; g_fds[1].flags = O_WRONLY; g_fds[1].is_std = 1;
    g_fds[2].handle = GetStdHandle(EZ_STD_ERROR_HANDLE);
    g_fds[2].open = 1; g_fds[2].flags = O_WRONLY; g_fds[2].is_std = 1;
}

static int win32_err_to_errno(unsigned long err)
{
    switch (err) {
    case EZ_ERROR_FILE_NOT_FOUND:
    case EZ_ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case EZ_ERROR_ACCESS_DENIED:
        return EACCES;
    case EZ_ERROR_FILE_EXISTS:
    case EZ_ERROR_ALREADY_EXISTS:
        return EEXIST;
    default:
        return EIO;
    }
}

static long long filetime_to_unix_nsec(EZ_FILETIME ft, long long *out_sec, long *out_nsec)
{
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    const unsigned long long EPOCH_DIFF = 116444736000000000ULL;
    if (t < EPOCH_DIFF) {
        *out_sec = 0;
        *out_nsec = 0;
        return 0;
    }
    unsigned long long adj = t - EPOCH_DIFF;
    *out_sec = (long long)(adj / 10000000ULL);
    *out_nsec = (long)((adj % 10000000ULL) * 100ULL);
    return *out_sec;
}

static long long win32_sys_read(int fd, void *buf, size_t count)
{
    init_fds();
    if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
    if (count == 0) return 0;
    unsigned long chunk = (count > 0x7fffffffULL) ? 0x7fffffffUL : (unsigned long)count;
    unsigned long read_bytes = 0;
    if (!ReadFile(g_fds[fd].handle, buf, chunk, &read_bytes, NULL)) {
        unsigned long err = GetLastError();
        if (err == EZ_ERROR_BROKEN_PIPE || err == EZ_ERROR_HANDLE_EOF) return 0;
        return -win32_err_to_errno(err);
    }
    return (long long)read_bytes;
}

static long long win32_sys_write(int fd, const void *buf, size_t count)
{
    init_fds();
    if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
    if (count == 0) return 0;
    if (g_fds[fd].flags & O_APPEND) {
        SetFilePointerEx(g_fds[fd].handle, 0, NULL, 2 /* FILE_END */);
    }
    unsigned long chunk = (count > 0x7fffffffULL) ? 0x7fffffffUL : (unsigned long)count;
    unsigned long written = 0;
    if (!WriteFile(g_fds[fd].handle, buf, chunk, &written, NULL)) {
        return -win32_err_to_errno(GetLastError());
    }
    return (long long)written;
}

static long long win32_sys_open(const char *path, int flags, int mode)
{
    (void)mode;
    init_fds();
    if (!path) return -EFAULT;

    /* Redirect /dev/null to Windows NUL device */
    if (strcmp(path, "/dev/null") == 0) {
        path = "NUL";
    }

    int fd = -1;
    for (int i = 3; i < EZ_MAX_FDS; i++) {
        if (!g_fds[i].open) {
            fd = i;
            break;
        }
    }
    if (fd < 0) return -EMFILE;

    unsigned long access = 0;
    int accmode = flags & O_ACCMODE;
    if (accmode == O_RDONLY) access = EZ_GENERIC_READ;
    else if (accmode == O_WRONLY) access = EZ_GENERIC_WRITE;
    else access = EZ_GENERIC_READ | EZ_GENERIC_WRITE;

    unsigned long disp = EZ_OPEN_EXISTING;
    if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
        disp = EZ_CREATE_NEW;
    } else if ((flags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) {
        disp = EZ_CREATE_ALWAYS;
    } else if (flags & O_CREAT) {
        disp = EZ_OPEN_ALWAYS;
    } else if (flags & O_TRUNC) {
        disp = EZ_TRUNCATE_EXISTING;
    }

    EZ_HANDLE h = CreateFileA(path, access, EZ_FILE_SHARE_READ | EZ_FILE_SHARE_WRITE,
                              NULL, disp, EZ_FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == EZ_INVALID_HANDLE_VALUE) {
        return -win32_err_to_errno(GetLastError());
    }

    g_fds[fd].handle = h;
    g_fds[fd].open = 1;
    g_fds[fd].flags = flags;
    g_fds[fd].is_std = 0;
    return fd;
}

static long long win32_sys_close(int fd)
{
    init_fds();
    if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
    if (!g_fds[fd].is_std && g_fds[fd].handle != EZ_INVALID_HANDLE_VALUE) {
        CloseHandle(g_fds[fd].handle);
    }
    g_fds[fd].open = 0;
    g_fds[fd].handle = EZ_INVALID_HANDLE_VALUE;
    return 0;
}

static long long win32_sys_fstat_handle(EZ_HANDLE h, struct kstat *kst)
{
    memset(kst, 0, sizeof(*kst));
    unsigned long ftype = GetFileType(h);
    if (ftype == EZ_FILE_TYPE_CHAR) {
        kst->st_mode = S_IFCHR | 0666;
        kst->st_nlink = 1;
        kst->st_blksize = 4096;
        return 0;
    }
    if (ftype == EZ_FILE_TYPE_PIPE) {
        kst->st_mode = S_IFIFO | 0666;
        kst->st_nlink = 1;
        kst->st_blksize = 4096;
        return 0;
    }

    EZ_BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(h, &info)) {
        return -win32_err_to_errno(GetLastError());
    }
    kst->st_dev = info.dwVolumeSerialNumber;
    kst->st_ino = ((ino_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
    kst->st_nlink = info.nNumberOfLinks ? info.nNumberOfLinks : 1;
    kst->st_mode = (info.dwFileAttributes & EZ_FILE_ATTRIBUTE_DIRECTORY)
        ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    kst->st_size = ((off_t)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    kst->st_blksize = 4096;
    kst->st_blocks = (kst->st_size + 511) / 512;

    long long sec = 0;
    long nsec = 0;
    filetime_to_unix_nsec(info.ftLastAccessTime, &sec, &nsec);
    kst->st_atime_sec = (long)sec; kst->st_atime_nsec = nsec;
    filetime_to_unix_nsec(info.ftLastWriteTime, &sec, &nsec);
    kst->st_mtime_sec = (long)sec; kst->st_mtime_nsec = nsec;
    filetime_to_unix_nsec(info.ftCreationTime, &sec, &nsec);
    kst->st_ctime_sec = (long)sec; kst->st_ctime_nsec = nsec;
    return 0;
}

static long long win32_sys_stat_path(const char *path, struct kstat *kst)
{
    if (!path) return -EFAULT;
    EZ_WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, 0 /* GetFileExInfoStandard */, &data)) {
        return -win32_err_to_errno(GetLastError());
    }
    memset(kst, 0, sizeof(*kst));
    kst->st_nlink = 1;
    kst->st_mode = (data.dwFileAttributes & EZ_FILE_ATTRIBUTE_DIRECTORY)
        ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    kst->st_size = ((off_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
    kst->st_blksize = 4096;
    kst->st_blocks = (kst->st_size + 511) / 512;

    long long sec = 0;
    long nsec = 0;
    filetime_to_unix_nsec(data.ftLastAccessTime, &sec, &nsec);
    kst->st_atime_sec = (long)sec; kst->st_atime_nsec = nsec;
    filetime_to_unix_nsec(data.ftLastWriteTime, &sec, &nsec);
    kst->st_mtime_sec = (long)sec; kst->st_mtime_nsec = nsec;
    filetime_to_unix_nsec(data.ftCreationTime, &sec, &nsec);
    kst->st_ctime_sec = (long)sec; kst->st_ctime_nsec = nsec;
    return 0;
}

long long __ez_win32_syscall(long n,
                             long long a1, long long a2, long long a3,
                             long long a4, long long a5, long long a6)
{
    (void)a5; (void)a6;
    init_fds();

    switch (n) {
    case SYS_exit:
    case SYS_exit_group:
        ExitProcess((unsigned int)a1);
        return 0;

    case SYS_read:
        return win32_sys_read((int)a1, (void *)a2, (size_t)a3);

    case SYS_write:
        return win32_sys_write((int)a1, (const void *)a2, (size_t)a3);

    case SYS_readv: {
        const struct iovec *iov = (const struct iovec *)a2;
        int iovcnt = (int)a3;
        long long total = 0;
        for (int i = 0; i < iovcnt; i++) {
            if (iov[i].iov_len == 0) continue;
            long long r = win32_sys_read((int)a1, iov[i].iov_base, iov[i].iov_len);
            if (r < 0) return total > 0 ? total : r;
            total += r;
            if ((size_t)r < iov[i].iov_len) break;
        }
        return total;
    }

    case SYS_writev: {
        const struct iovec *iov = (const struct iovec *)a2;
        int iovcnt = (int)a3;
        long long total = 0;
        for (int i = 0; i < iovcnt; i++) {
            if (iov[i].iov_len == 0) continue;
            long long r = win32_sys_write((int)a1, iov[i].iov_base, iov[i].iov_len);
            if (r < 0) return total > 0 ? total : r;
            total += r;
            if ((size_t)r < iov[i].iov_len) break;
        }
        return total;
    }

    case SYS_open:
        return win32_sys_open((const char *)a1, (int)a2, (int)a3);

    case SYS_openat:
        return win32_sys_open((const char *)a2, (int)a3, (int)a4);

    case SYS_close:
        return win32_sys_close((int)a1);

    case SYS_lseek: {
        int fd = (int)a1;
        if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
        long long new_pos = 0;
        if (!SetFilePointerEx(g_fds[fd].handle, a2, &new_pos, (unsigned long)a3)) {
            return -ESPIPE;
        }
        return new_pos;
    }

    case SYS_ioctl: {
        int fd = (int)a1;
        unsigned long req = (unsigned long)a2;
        if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
        if (req == TIOCGWINSZ) {
            unsigned long mode = 0;
            if (GetConsoleMode(g_fds[fd].handle, &mode)) {
                struct winsize *ws = (struct winsize *)a3;
                if (ws) { ws->ws_row = 24; ws->ws_col = 80; ws->ws_xpixel = 0; ws->ws_ypixel = 0; }
                return 0;
            }
            return -ENOTTY;
        }
        return -ENOSYS;
    }

    case SYS_fstat: {
        int fd = (int)a1;
        if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
        return win32_sys_fstat_handle(g_fds[fd].handle, (struct kstat *)a2);
    }

    case SYS_stat:
    case SYS_lstat:
        return win32_sys_stat_path((const char *)a1, (struct kstat *)a2);

    case SYS_newfstatat: {
        int fd = (int)a1;
        const char *path = (const char *)a2;
        struct kstat *kst = (struct kstat *)a3;
        int flag = (int)a4;
        if ((flag & AT_EMPTY_PATH) && (!path || !*path)) {
            if (fd < 0 || fd >= EZ_MAX_FDS || !g_fds[fd].open) return -EBADF;
            return win32_sys_fstat_handle(g_fds[fd].handle, kst);
        }
        return win32_sys_stat_path(path, kst);
    }

    case SYS_statx:
        return -ENOSYS;

    case SYS_access: {
        const char *path = (const char *)a1;
        EZ_WIN32_FILE_ATTRIBUTE_DATA data;
        if (!path) return -EFAULT;
        if (!GetFileAttributesExA(path, 0, &data)) return -win32_err_to_errno(GetLastError());
        return 0;
    }

    case SYS_faccessat: {
        const char *path = (const char *)a2;
        EZ_WIN32_FILE_ATTRIBUTE_DATA data;
        if (!path) return -EFAULT;
        if (!GetFileAttributesExA(path, 0, &data)) return -win32_err_to_errno(GetLastError());
        return 0;
    }

    case SYS_unlink:
        return DeleteFileA((const char *)a1) ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_unlinkat:
        if ((int)a3 & AT_REMOVEDIR) {
            return RemoveDirectoryA((const char *)a2) ? 0 : -win32_err_to_errno(GetLastError());
        }
        return DeleteFileA((const char *)a2) ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_rename:
        return MoveFileExA((const char *)a1, (const char *)a2, 1 /* MOVEFILE_REPLACE_EXISTING */)
            ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_renameat:
        return MoveFileExA((const char *)a2, (const char *)a4, 1 /* MOVEFILE_REPLACE_EXISTING */)
            ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_mkdir:
        return CreateDirectoryA((const char *)a1, NULL) ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_rmdir:
        return RemoveDirectoryA((const char *)a1) ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_getcwd: {
        char *buf = (char *)a1;
        unsigned long len = (unsigned long)a2;
        unsigned long res = GetCurrentDirectoryA(len, buf);
        if (res == 0) return -ENOENT;
        if (res >= len) return -ERANGE;
        return (long long)(res + 1);
    }

    case SYS_chdir:
        return SetCurrentDirectoryA((const char *)a1) ? 0 : -win32_err_to_errno(GetLastError());

    case SYS_dup:
    case SYS_dup2:
    case SYS_dup3: {
        int oldfd = (int)a1;
        if (oldfd < 0 || oldfd >= EZ_MAX_FDS || !g_fds[oldfd].open) return -EBADF;
        int newfd = -1;
        if (n == SYS_dup) {
            for (int i = 0; i < EZ_MAX_FDS; i++) {
                if (!g_fds[i].open) { newfd = i; break; }
            }
            if (newfd < 0) return -EMFILE;
        } else {
            newfd = (int)a2;
            if (newfd < 0 || newfd >= EZ_MAX_FDS) return -EBADF;
            if (newfd == oldfd) return newfd;
            if (g_fds[newfd].open) win32_sys_close(newfd);
        }
        EZ_HANDLE dup_h = EZ_INVALID_HANDLE_VALUE;
        EZ_HANDLE proc = GetCurrentProcess();
        if (!DuplicateHandle(proc, g_fds[oldfd].handle, proc, &dup_h, 0, 1, 2 /* DUPLICATE_SAME_ACCESS */)) {
            return -EBADF;
        }
        g_fds[newfd].handle = dup_h;
        g_fds[newfd].open = 1;
        g_fds[newfd].flags = g_fds[oldfd].flags;
        g_fds[newfd].is_std = 0;
        return newfd;
    }

    case SYS_fcntl:
        return 0;

    case SYS_clock_gettime: {
        clockid_t clk = (clockid_t)a1;
        struct timespec *ts = (struct timespec *)a2;
        if (!ts) return -EFAULT;
        if (clk == CLOCK_MONOTONIC) {
            long long freq = 1, cnt = 0;
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&cnt);
            ts->tv_sec = (time_t)(cnt / freq);
            ts->tv_nsec = (long)(((cnt % freq) * 1000000000LL) / freq);
            return 0;
        }
        EZ_FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        long long sec = 0;
        long nsec = 0;
        filetime_to_unix_nsec(ft, &sec, &nsec);
        ts->tv_sec = (time_t)sec;
        ts->tv_nsec = nsec;
        return 0;
    }

    case SYS_gettimeofday: {
        struct timeval *tv = (struct timeval *)a1;
        if (tv) {
            EZ_FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            long long sec = 0;
            long nsec = 0;
            filetime_to_unix_nsec(ft, &sec, &nsec);
            tv->tv_sec = (time_t)sec;
            tv->tv_usec = (suseconds_t)(nsec / 1000);
        }
        return 0;
    }

    case SYS_nanosleep:
    case SYS_clock_nanosleep: {
        const struct timespec *req = (n == SYS_nanosleep)
            ? (const struct timespec *)a1
            : (const struct timespec *)a3;
        if (req) {
            unsigned long ms = (unsigned long)(req->tv_sec * 1000LL + req->tv_nsec / 1000000L);
            Sleep(ms);
        }
        return 0;
    }

    case SYS_getpid:
        return (long long)GetCurrentProcessId();

    case SYS_gettid:
        return (long long)GetCurrentThreadId();

    case SYS_getuid:
    case SYS_geteuid:
    case SYS_getgid:
    case SYS_getegid:
        return 1000;

    case SYS_getrandom: {
        unsigned char *buf = (unsigned char *)a1;
        size_t len = (size_t)a2;
        long long qpc = 0;
        QueryPerformanceCounter(&qpc);
        uint64_t s = (uint64_t)qpc ^ ((uint64_t)GetCurrentProcessId() << 32);
        for (size_t i = 0; i < len; i++) {
            s ^= s << 13; s ^= s >> 7; s ^= s << 17;
            buf[i] = (unsigned char)(s & 0xff);
        }
        return (long long)len;
    }

    case SYS_set_tid_address:
        return (long long)GetCurrentThreadId();

    case SYS_brk:
        return -ENOMEM; /* Forces mallocng to use mmap/VirtualAlloc */

    case SYS_sched_yield:
        Sleep(0);
        return 0;

    case SYS_futex:
    case SYS_rt_sigaction:
    case SYS_rt_sigprocmask:
        return 0;

    case SYS_tkill:
    case SYS_tgkill:
    case SYS_kill:
        if ((int)a2 == 6 /* SIGABRT */ || (int)a3 == 6) {
            ExitProcess(134);
        }
        return 0;

    default:
        return -ENOSYS;
    }
}
