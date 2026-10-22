#ifndef _EZPACKER_SETJMP_H
#define _EZPACKER_SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long jmp_buf[32];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_SETJMP_H
