#ifndef _EZPACKER_FENV_H
#define _EZPACKER_FENV_H
typedef unsigned short fexcept_t;
typedef struct { unsigned short __status; unsigned short __control; } fenv_t;
#define FE_ALL_EXCEPT 0
#endif
