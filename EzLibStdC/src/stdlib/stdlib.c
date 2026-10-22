#include "os/os_support.h"
#include <stddef.h>

void *malloc(size_t size) {
    return ez_os_alloc(size);
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = ez_os_alloc(total);
    if (ptr) {
        char *p = (char *)ptr;
        for (size_t i = 0; i < total; ++i) p[i] = 0;
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    return ez_os_realloc(ptr, size);
}

void free(void *ptr) {
    ez_os_free(ptr);
}

void abort(void) {
    ez_os_exit(3);
}

void exit(int status) {
    ez_os_exit(status);
}

void _Exit(int status) {
    ez_os_exit(status);
}

static void (*g_atexit_funcs[32])(void);
static int g_atexit_count = 0;

int atexit(void (*func)(void)) {
    if (g_atexit_count < 32 && func) {
        g_atexit_funcs[g_atexit_count++] = func;
        return 0;
    }
    return -1;
}

char *getenv(const char *name) {
    (void)name;
    return NULL;
}

int system(const char *command) {
    (void)command;
    return -1;
}

int abs(int j) { return j < 0 ? -j : j; }
long labs(long j) { return j < 0 ? -j : j; }
long long llabs(long long j) { return j < 0 ? -j : j; }

int atoi(const char *nptr) {
    int res = 0;
    int sign = 1;
    while (*nptr == ' ' || (*nptr >= 9 && *nptr <= 13)) ++nptr;
    if (*nptr == '-') { sign = -1; ++nptr; }
    else if (*nptr == '+') ++nptr;
    while (*nptr >= '0' && *nptr <= '9') {
        res = res * 10 + (*nptr - '0');
        ++nptr;
    }
    return res * sign;
}

long atol(const char *nptr) { return (long)atoi(nptr); }
long long atoll(const char *nptr) { return (long long)atoi(nptr); }
double atof(const char *nptr) { return (double)atoi(nptr); }

long strtol(const char *nptr, char **endptr, int base) {
    long res = 0;
    int sign = 1;
    while (*nptr == ' ' || (*nptr >= 9 && *nptr <= 13)) ++nptr;
    if (*nptr == '-') { sign = -1; ++nptr; }
    else if (*nptr == '+') ++nptr;
    if (base == 0) {
        if (*nptr == '0') {
            if (nptr[1] == 'x' || nptr[1] == 'X') { base = 16; nptr += 2; }
            else base = 8;
        } else base = 10;
    }
    while (*nptr) {
        int v;
        if (*nptr >= '0' && *nptr <= '9') v = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'f') v = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'F') v = *nptr - 'A' + 10;
        else break;
        if (v >= base) break;
        res = res * base + v;
        ++nptr;
    }
    if (endptr) *endptr = (char *)nptr;
    return res * sign;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

long long strtoll(const char *nptr, char **endptr, int base) {
    return (long long)strtol(nptr, endptr, base);
}

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    return (unsigned long long)strtol(nptr, endptr, base);
}

static unsigned long g_next_rand = 1;

int rand(void) {
    g_next_rand = g_next_rand * 1103515245 + 12345;
    return (int)((g_next_rand / 65536) % 32768);
}

void srand(unsigned int seed) {
    g_next_rand = seed;
}

static void swap_bytes(char *a, char *b, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        char t = a[i];
        a[i] = b[i];
        b[i] = t;
    }
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    if (nmemb < 2 || size == 0) return;
    char *arr = (char *)base;
    for (size_t i = 0; i < nmemb - 1; ++i) {
        for (size_t j = i + 1; j < nmemb; ++j) {
            if (compar(arr + i * size, arr + j * size) > 0) {
                swap_bytes(arr + i * size, arr + j * size, size);
            }
        }
    }
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    size_t low = 0;
    size_t high = nmemb;
    const char *arr = (const char *)base;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        int cmp = compar(key, arr + mid * size);
        if (cmp < 0) high = mid;
        else if (cmp > 0) low = mid + 1;
        else return (void *)(arr + mid * size);
    }
    return NULL;
}
