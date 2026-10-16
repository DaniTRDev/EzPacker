#ifndef CHIBICC_SHIM_STRINGS_H
#define CHIBICC_SHIM_STRINGS_H

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

static inline char *chibicc_shim_strndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    if (p) {
        memcpy(p, s, n);
        p[n] = '\0';
    }
    return p;
}
#define strndup chibicc_shim_strndup

#endif

#endif // CHIBICC_SHIM_STRINGS_H
