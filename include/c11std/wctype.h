#ifndef _EZPACKER_WCTYPE_H
#define _EZPACKER_WCTYPE_H
#include <wchar.h>
typedef unsigned int wctype_t;
typedef unsigned int wctrans_t;
int iswalpha(wint_t wc);
int iswdigit(wint_t wc);
int iswspace(wint_t wc);
wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);
#endif
