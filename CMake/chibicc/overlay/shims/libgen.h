#ifndef CHIBICC_SHIM_LIBGEN_H
#define CHIBICC_SHIM_LIBGEN_H

#ifdef _WIN32
#include <string.h>

static inline char *chibicc_shim_dirname(char *path) {
    if (!path || !*path)
        return ".";
    char *p = strrchr(path, '/');
    char *p2 = strrchr(path, '\\');
    if (p2 && (!p || p2 > p))
        p = p2;
    if (!p)
        return ".";
    if (p == path)
        return "/";
    *p = '\0';
    return path;
}

#define dirname chibicc_shim_dirname
#else
#include_next <libgen.h>
#endif

#endif // CHIBICC_SHIM_LIBGEN_H
