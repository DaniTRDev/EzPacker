#ifndef _EZPACKER_MATH_H
#define _EZPACKER_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL  (__builtin_huge_val())
#define INFINITY  (__builtin_inff())
#define NAN       (__builtin_nanf(""))

double fabs(double x);
float fabsf(float x);
long double fabsl(long double x);

double sqrt(double x);
float sqrtf(float x);

double pow(double base, double exp);
float powf(float base, float exp);

double sin(double x);
double cos(double x);
double tan(double x);

double exp(double x);
double log(double x);

double floor(double x);
double ceil(double x);
double round(double x);

double fmin(double x, double y);
double fmax(double x, double y);

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_MATH_H
