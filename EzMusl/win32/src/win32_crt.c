#include <stdint.h>
#include <stddef.h>
#include <elf.h>
#include "pthread_impl.h"
#include "libc.h"

__declspec(dllimport) char *__stdcall GetCommandLineA(void);
__declspec(dllimport) char *__stdcall GetEnvironmentStringsA(void);
__declspec(dllimport) int __stdcall FreeEnvironmentStringsA(char *penv);
__declspec(dllimport) void *__stdcall HeapAlloc(void *hHeap, unsigned long dwFlags, size_t dwBytes);
__declspec(dllimport) void *__stdcall GetProcessHeap(void);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *lpPerformanceCount);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);

extern int main(int argc, char **argv, char **envp);
extern int __libc_start_main(int (*)(int, char **, char **), int, char **,
                             void (*)(void), void (*)(void), void (*)(void));

static struct pthread g_fallback_thread;
static uintptr_t g_win32_tp = (uintptr_t)&g_fallback_thread;

uintptr_t __ez_win32_get_tp(void)
{
    return g_win32_tp;
}

int __set_thread_area(void *p)
{
    g_win32_tp = (uintptr_t)p;
    return 0;
}

/* MinGW GCC emits a call to __main() inside main() for C++ ctors. */
void __main(void)
{
}

static void parse_cmdline_and_env(int *out_argc, char ***out_argv)
{
    const char *cmd = GetCommandLineA();
    void *heap = GetProcessHeap();

    /* First count worst-case length and build an argv array on process heap */
    size_t cmd_len = 0;
    while (cmd && cmd[cmd_len]) cmd_len++;

    char *buf = (char *)HeapAlloc(heap, 8 /* HEAP_ZERO_MEMORY */, cmd_len + 16);
    int max_args = 256;
    char **argv_raw = (char **)HeapAlloc(heap, 8, sizeof(char *) * max_args);

    int argc = 0;
    const char *p = cmd ? cmd : "";
    char *dst = buf;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (argc + 1 >= max_args) break;
        argv_raw[argc++] = dst;

        int in_quotes = 0;
        while (*p) {
            if (*p == '"') {
                in_quotes = !in_quotes;
                p++;
            } else if (!in_quotes && (*p == ' ' || *p == '\t')) {
                break;
            } else if (*p == '\\' && p[1] == '"') {
                *dst++ = '"';
                p += 2;
            } else {
                *dst++ = *p++;
            }
        }
        *dst++ = '\0';
    }

    if (argc == 0) {
        argv_raw[0] = (char *)"ezpacker_app";
        argc = 1;
    }

    /* Count environment strings */
    char *env_block = GetEnvironmentStringsA();
    int envc = 0;
    if (env_block) {
        const char *ep = env_block;
        while (*ep) {
            if (ep[0] != '=') envc++;
            while (*ep) ep++;
            ep++;
        }
    }

    /* Stack layout expected by musl's __libc_start_main:
       argv[0..argc-1], NULL, envp[0..envc-1], NULL, auxv pairs..., AT_NULL, 0 */
    size_t total_slots = (size_t)argc + 1 + (size_t)envc + 1 + 16;
    uintptr_t *vec = (uintptr_t *)HeapAlloc(heap, 8, total_slots * sizeof(uintptr_t));

    int idx = 0;
    for (int i = 0; i < argc; i++) {
        vec[idx++] = (uintptr_t)argv_raw[i];
    }
    vec[idx++] = 0; /* argv terminator */

    if (env_block) {
        const char *ep = env_block;
        while (*ep) {
            size_t len = 0;
            while (ep[len]) len++;
            if (ep[0] != '=') {
                char *copy = (char *)HeapAlloc(heap, 0, len + 1);
                for (size_t j = 0; j <= len; j++) copy[j] = ep[j];
                vec[idx++] = (uintptr_t)copy;
            }
            ep += len + 1;
        }
        FreeEnvironmentStringsA(env_block);
    }
    vec[idx++] = 0; /* envp terminator */

    static unsigned char rand_seed[16];
    long long qpc = 0;
    QueryPerformanceCounter(&qpc);
    unsigned long pid = GetCurrentProcessId();
    for (int i = 0; i < 8; i++) rand_seed[i] = (unsigned char)((qpc >> (i * 8)) & 0xff);
    for (int i = 0; i < 4; i++) rand_seed[8 + i] = (unsigned char)((pid >> (i * 8)) & 0xff);

    vec[idx++] = AT_PAGESZ; vec[idx++] = 4096;
    vec[idx++] = AT_UID;    vec[idx++] = 1000;
    vec[idx++] = AT_EUID;   vec[idx++] = 1000;
    vec[idx++] = AT_GID;    vec[idx++] = 1000;
    vec[idx++] = AT_EGID;   vec[idx++] = 1000;
    vec[idx++] = AT_RANDOM; vec[idx++] = (uintptr_t)rand_seed;
    vec[idx++] = AT_EXECFN; vec[idx++] = (uintptr_t)argv_raw[0];
    vec[idx++] = AT_NULL;   vec[idx++] = 0;

    *out_argc = argc;
    *out_argv = (char **)vec;
}

void mainCRTStartup(void)
{
    g_fallback_thread.self = &g_fallback_thread;
    g_fallback_thread.tid = 1;
    g_win32_tp = (uintptr_t)&g_fallback_thread;

    int argc = 0;
    char **argv = NULL;
    parse_cmdline_and_env(&argc, &argv);

    __libc_start_main(main, argc, argv, 0, 0, 0);
}

void WinMainCRTStartup(void)
{
    mainCRTStartup();
}
