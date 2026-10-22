#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    if (d < s) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
    } else if (d > s) {
        for (size_t i = n; i > 0; --i) d[i - 1] = s[i - 1];
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; ++i) {
        p[i] = val;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; ++i) {
        if (p[i] == val) return (void *)(p + i);
    }
    return NULL;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) ++len;
    return len;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    while ((dest[i] = src[i]) != '\0') ++i;
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; ++i) dest[i] = src[i];
    for (; i < n; ++i) dest[i] = '\0';
    return dest;
}

char *strcat(char *dest, const char *src) {
    size_t dlen = strlen(dest);
    size_t i = 0;
    while ((dest[dlen + i] = src[i]) != '\0') ++i;
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    size_t dlen = strlen(dest);
    size_t i = 0;
    for (; i < n && src[i] != '\0'; ++i) dest[dlen + i] = src[i];
    dest[dlen + i] = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i] || s1[i] == '\0')
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

char *strchr(const char *s, int c) {
    char ch = (char)c;
    while (*s != ch) {
        if (*s == '\0') return NULL;
        ++s;
    }
    return (char *)s;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    char ch = (char)c;
    do {
        if (*s == ch) last = s;
    } while (*s++);
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; ++haystack) {
        if (*haystack == *needle) {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && *h == *n) {
                ++h;
                ++n;
            }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

size_t strspn(const char *s, const char *accept) {
    size_t count = 0;
    while (*s && strchr(accept, *s++)) ++count;
    return count;
}

size_t strcspn(const char *s, const char *reject) {
    size_t count = 0;
    while (*s && !strchr(reject, *s++)) ++count;
    return count;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        if (strchr(accept, *s)) return (char *)s;
        ++s;
    }
    return NULL;
}

static char *g_strtok_pos = NULL;

char *strtok(char *str, const char *delim) {
    if (str) g_strtok_pos = str;
    if (!g_strtok_pos) return NULL;

    g_strtok_pos += strspn(g_strtok_pos, delim);
    if (!*g_strtok_pos) return NULL;

    char *token = g_strtok_pos;
    g_strtok_pos += strcspn(g_strtok_pos, delim);
    if (*g_strtok_pos) {
        *g_strtok_pos++ = '\0';
    } else {
        g_strtok_pos = NULL;
    }
    return token;
}

char *strerror(int errnum) {
    (void)errnum;
    return "Unknown error";
}
