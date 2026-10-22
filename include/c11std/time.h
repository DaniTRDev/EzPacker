#ifndef _EZPACKER_TIME_H
#define _EZPACKER_TIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCKS_PER_SEC ((clock_t)1000)

typedef long long clock_t;
typedef long long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

clock_t clock(void);
time_t time(time_t *tloc);
double difftime(time_t time1, time_t time0);
time_t mktime(struct tm *tm);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_TIME_H
