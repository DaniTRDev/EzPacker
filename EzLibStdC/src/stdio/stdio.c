#include "os/os_support.h"
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

struct _FILE {
    int fd;
};

static struct _FILE g_stdin_file = { 0 };
static struct _FILE g_stdout_file = { 1 };
static struct _FILE g_stderr_file = { 2 };

struct _FILE *stdin = &g_stdin_file;
struct _FILE *stdout = &g_stdout_file;
struct _FILE *stderr = &g_stderr_file;

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    size_t written = 0;
    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            if (str && written + 1 < size) str[written] = *p;
            ++written;
            continue;
        }
        ++p;
        if (!*p) break;
        if (*p == '%') {
            if (str && written + 1 < size) str[written] = '%';
            ++written;
            continue;
        }
        if (*p == 'c') {
            int c = va_arg(ap, int);
            if (str && written + 1 < size) str[written] = (char)c;
            ++written;
            continue;
        }
        if (*p == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s) {
                if (str && written + 1 < size) str[written] = *s;
                ++written;
                ++s;
            }
            continue;
        }
        if (*p == 'd' || *p == 'i') {
            long long val = va_arg(ap, int);
            if (val < 0) {
                if (str && written + 1 < size) str[written] = '-';
                ++written;
                val = -val;
            }
            char buf[32];
            int idx = 0;
            if (val == 0) buf[idx++] = '0';
            else {
                char rev[32];
                int r = 0;
                while (val > 0) {
                    rev[r++] = (char)('0' + (val % 10));
                    val /= 10;
                }
                while (r > 0) buf[idx++] = rev[--r];
            }
            buf[idx] = '\0';
            for (int k = 0; k < idx; ++k) {
                if (str && written + 1 < size) str[written] = buf[k];
                ++written;
            }
            continue;
        }
        if (*p == 'u' || *p == 'x' || *p == 'X' || *p == 'p') {
            unsigned long long uval = (*p == 'p') ? (unsigned long long)va_arg(ap, void *) : (unsigned long long)va_arg(ap, unsigned int);
            int base = (*p == 'u') ? 10 : 16;
            char buf[32];
            int idx = 0;
            if (*p == 'p') {
                if (str && written + 1 < size) str[written] = '0';
                ++written;
                if (str && written + 1 < size) str[written] = 'x';
                ++written;
            }
            if (uval == 0) buf[idx++] = '0';
            else {
                char rev[32];
                int r = 0;
                while (uval > 0) {
                    int d = uval % base;
                    rev[r++] = (char)(d < 10 ? '0' + d : (*p == 'X' ? 'A' : 'a') + (d - 10));
                    uval /= base;
                }
                while (r > 0) buf[idx++] = rev[--r];
            }
            buf[idx] = '\0';
            for (int k = 0; k < idx; ++k) {
                if (str && written + 1 < size) str[written] = buf[k];
                ++written;
            }
            continue;
        }
        // Generic fallback for unhandled specifier
        if (str && written + 1 < size) str[written] = *p;
        ++written;
    }
    if (str && size > 0) {
        if (written < size) str[written] = '\0';
        else str[size - 1] = '\0';
    }
    return (int)written;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    return vsnprintf(str, (size_t)-1, format, ap);
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *format, va_list ap) {
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), format, ap);
    if (len > 0) {
        ez_os_write_stdout(buf, len < (int)sizeof(buf) ? (size_t)len : sizeof(buf) - 1);
    }
    return len;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

int puts(const char *s) {
    size_t len = strlen(s);
    ez_os_write_stdout(s, len);
    ez_os_write_stdout("\n", 1);
    return 0;
}

int putchar(int c) {
    char ch = (char)c;
    ez_os_write_stdout(&ch, 1);
    return c;
}

int fputs(const char *s, struct _FILE *stream) {
    size_t len = strlen(s);
    if (stream == stderr) ez_os_write_stderr(s, len);
    else ez_os_write_stdout(s, len);
    return 0;
}

int fputc(int c, struct _FILE *stream) {
    char ch = (char)c;
    if (stream == stderr) ez_os_write_stderr(&ch, 1);
    else ez_os_write_stdout(&ch, 1);
    return c;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, struct _FILE *stream) {
    size_t total = size * nmemb;
    if (stream == stderr) ez_os_write_stderr(ptr, total);
    else ez_os_write_stdout(ptr, total);
    return nmemb;
}

size_t fread(void *ptr, size_t size, size_t nmemb, struct _FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    return 0;
}

struct _FILE *fopen(const char *pathname, const char *mode) {
    (void)pathname; (void)mode;
    return NULL;
}

int fclose(struct _FILE *stream) {
    (void)stream;
    return 0;
}
