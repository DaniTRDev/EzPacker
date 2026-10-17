#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Freestanding SQLite3-style kernel snippet */

typedef struct sqlite3_vfs sqlite3_vfs;
typedef struct sqlite3_file sqlite3_file;

struct sqlite3_file {
    int flags;
};

struct sqlite3_vfs {
    int iVersion;
    int szOsFile;
    int mxPathname;
    const char *zName;
    int (*xOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*, int flags, int *pOutFlags);
};

typedef struct {
    int rc;
    char *zErrMsg;
} ParseState;

static int sqlite3_format_error(ParseState *pParse, const char *zFormat, ...) {
    va_list ap;
    va_start(ap, zFormat);

    if (__builtin_expect(pParse != NULL, 1)) {
        pParse->rc = 1;
    }

    /* Simulate reading an integer argument */
    int code = va_arg(ap, int);
    pParse->rc += code;

    /* C23 / AVX-512 FP16 literal test */
    float f = 0.0f16;
    if (f > 0.0f) {
        pParse->rc += 42;
    }

    va_end(ap);
    return pParse->rc;
}

int main(void) {
    ParseState ps;
    ps.rc = 0;
    ps.zErrMsg = NULL;

    int res = sqlite3_format_error(&ps, "Error code: %d", 100);
    if (__builtin_expect(res == 101, 1)) {
        return 0;
    }
    return 1;
}
