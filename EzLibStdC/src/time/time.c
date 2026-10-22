#include <stddef.h>

long long clock(void) {
    return 0;
}

long long time(long long *tloc) {
    long long t = 0;
    if (tloc) *tloc = t;
    return t;
}

double difftime(long long time1, long long time0) {
    return (double)(time1 - time0);
}
