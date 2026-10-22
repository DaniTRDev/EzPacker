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
#define EMIT_CHAR(c) do { \
    if (str && written + 1 < size) str[written] = (c); \
    ++written; \
} while (0)

    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            EMIT_CHAR(*p);
            continue;
        }
        ++p;
        if (!*p) break;
        if (*p == '%') {
            EMIT_CHAR('%');
            continue;
        }

        // Flags
        int left_align = 0;
        int zero_pad = 0;
        while (*p == '-' || *p == '0' || *p == '+' || *p == ' ') {
            if (*p == '-') left_align = 1;
            else if (*p == '0') zero_pad = 1;
            ++p;
        }
        if (left_align) zero_pad = 0;

        // Width
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            ++p;
        }

        // Precision
        int prec = -1;
        if (*p == '.') {
            ++p;
            prec = 0;
            while (*p >= '0' && *p <= '9') {
                prec = prec * 10 + (*p - '0');
                ++p;
            }
        }

        // Length
        int is_long = 0;
        while (*p == 'l' || *p == 'z' || *p == 'h') {
            if (*p == 'l') ++is_long;
            ++p;
        }

        if (*p == 'c') {
            int c = va_arg(ap, int);
            int pad = (width > 1) ? width - 1 : 0;
            if (!left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            EMIT_CHAR((char)c);
            if (left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            continue;
        }

        if (*p == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int slen = (int)strlen(s);
            if (prec >= 0 && prec < slen) slen = prec;
            int pad = (width > slen) ? width - slen : 0;
            if (!left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            for (int i = 0; i < slen; ++i) EMIT_CHAR(s[i]);
            if (left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            continue;
        }

        if (*p == 'd' || *p == 'i') {
            long long val = (is_long >= 2) ? va_arg(ap, long long)
                          : (is_long == 1) ? (long long)va_arg(ap, long)
                          : (long long)va_arg(ap, int);
            int is_neg = (val < 0);
            unsigned long long uval = is_neg ? (unsigned long long)(-val) : (unsigned long long)val;

            char digits[32];
            int dcount = 0;
            if (uval == 0) digits[dcount++] = '0';
            else {
                while (uval > 0) {
                    digits[dcount++] = (char)('0' + (uval % 10));
                    uval /= 10;
                }
            }
            int total_len = dcount + (is_neg ? 1 : 0);
            int pad = (width > total_len) ? width - total_len : 0;

            if (!left_align && !zero_pad) { while (pad-- > 0) EMIT_CHAR(' '); }
            if (is_neg) EMIT_CHAR('-');
            if (!left_align && zero_pad) { while (pad-- > 0) EMIT_CHAR('0'); }
            for (int i = dcount - 1; i >= 0; --i) EMIT_CHAR(digits[i]);
            if (left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            continue;
        }

        if (*p == 'u' || *p == 'x' || *p == 'X' || *p == 'p') {
            unsigned long long uval = (*p == 'p') ? (unsigned long long)va_arg(ap, void *)
                                    : (is_long >= 2) ? va_arg(ap, unsigned long long)
                                    : (is_long == 1) ? (unsigned long long)va_arg(ap, unsigned long)
                                    : (unsigned long long)va_arg(ap, unsigned int);
            int base = (*p == 'u') ? 10 : 16;
            char digits[32];
            int dcount = 0;
            if (uval == 0) digits[dcount++] = '0';
            else {
                while (uval > 0) {
                    int d = (int)(uval % base);
                    digits[dcount++] = (char)(d < 10 ? '0' + d : (*p == 'X' ? 'A' : 'a') + (d - 10));
                    uval /= base;
                }
            }
            int pfx_len = (*p == 'p') ? 2 : 0;
            int total_len = dcount + pfx_len;
            int pad = (width > total_len) ? width - total_len : 0;

            if (!left_align && !zero_pad) { while (pad-- > 0) EMIT_CHAR(' '); }
            if (*p == 'p') { EMIT_CHAR('0'); EMIT_CHAR('x'); }
            if (!left_align && zero_pad) { while (pad-- > 0) EMIT_CHAR('0'); }
            for (int i = dcount - 1; i >= 0; --i) EMIT_CHAR(digits[i]);
            if (left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            continue;
        }

        if (*p == 'f') {
            double fval = va_arg(ap, double);
            int is_neg = (fval < 0.0);
            if (is_neg) fval = -fval;
            if (prec < 0) prec = 6;

            long long int_part = (long long)fval;
            double frac_part = fval - (double)int_part;

            char buf[64];
            int bidx = 0;

            char int_digits[32];
            int idcount = 0;
            if (int_part == 0) int_digits[idcount++] = '0';
            else {
                long long tmp = int_part;
                while (tmp > 0) {
                    int_digits[idcount++] = (char)('0' + (tmp % 10));
                    tmp /= 10;
                }
            }
            while (idcount > 0) buf[bidx++] = int_digits[--idcount];

            if (prec > 0) {
                buf[bidx++] = '.';
                for (int i = 0; i < prec; ++i) {
                    frac_part *= 10.0;
                    int digit = (int)frac_part;
                    if (digit > 9) digit = 9;
                    buf[bidx++] = (char)('0' + digit);
                    frac_part -= (double)digit;
                }
            }
            buf[bidx] = '\0';

            int total_len = bidx + (is_neg ? 1 : 0);
            int pad = (width > total_len) ? width - total_len : 0;

            if (!left_align && !zero_pad) { while (pad-- > 0) EMIT_CHAR(' '); }
            if (is_neg) EMIT_CHAR('-');
            if (!left_align && zero_pad) { while (pad-- > 0) EMIT_CHAR('0'); }
            for (int i = 0; i < bidx; ++i) EMIT_CHAR(buf[i]);
            if (left_align) { while (pad-- > 0) EMIT_CHAR(' '); }
            continue;
        }

        EMIT_CHAR(*p);
    }
#undef EMIT_CHAR

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

int fprintf(struct _FILE *stream, const char *format, ...) {

    char buf[1024];

    va_list ap;

    va_start(ap, format);

    int res = vsnprintf(buf, sizeof(buf), format, ap);

    va_end(ap);

    fputs(buf, stream);

    return res;

}



int vfprintf(struct _FILE *stream, const char *format, va_list ap) {

    char buf[1024];

    int res = vsnprintf(buf, sizeof(buf), format, ap);

    fputs(buf, stream);

    return res;

}



int fflush(struct _FILE *stream) {

    (void)stream;

    return 0;

}



int fgetc(struct _FILE *stream) {

    (void)stream;

    return -1;

}



int getc(struct _FILE *stream) {

    return fgetc(stream);

}



int getchar(void) {

    return fgetc(stdin);

}



int putc(int c, struct _FILE *stream) {

    return fputc(c, stream);

}



char *fgets(char *s, int size, struct _FILE *stream) {

    (void)s; (void)size; (void)stream;

    return NULL;

}



int ungetc(int c, struct _FILE *stream) {

    (void)c; (void)stream;

    return -1;

}



int fseek(struct _FILE *stream, long offset, int whence) {

    (void)stream; (void)offset; (void)whence;

    return -1;

}



long ftell(struct _FILE *stream) {

    (void)stream;

    return -1L;

}



void rewind(struct _FILE *stream) {

    (void)stream;

}



int feof(struct _FILE *stream) {

    (void)stream;

    return 0;

}



int ferror(struct _FILE *stream) {

    (void)stream;

    return 0;

}



void clearerr(struct _FILE *stream) {

    (void)stream;

}



void perror(const char *s) {

    if (s && *s) {

        fputs(s, stderr);

        fputs(": Unknown error\n", stderr);

    }

}



int remove(const char *pathname) {

    (void)pathname;

    return -1;

}



int rename(const char *oldpath, const char *newpath) {

    (void)oldpath; (void)newpath;

    return -1;

}

