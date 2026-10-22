#ifndef _EZPACKER_THREADS_H
#define _EZPACKER_THREADS_H
#include <time.h>
typedef void *thrd_t;
typedef void *mtx_t;
typedef void *cnd_t;
typedef void *tss_t;
enum { thrd_success = 0, thrd_timedout, thrd_busy, thrd_error, thrd_nomem };
#endif
