#ifndef _EZPACKER_WCHAR_H
#define _EZPACKER_WCHAR_H
#include <stddef.h>
#include <stdio.h>
typedef struct { int __count; unsigned int __value; } mbstate_t;
#define WEOF ((wint_t)-1)
typedef unsigned int wint_t;
size_t wcslen(const wchar_t *s);
int wcscmp(const wchar_t *s1, const wchar_t *s2);
#endif
