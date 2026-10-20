// EzPacker YARP End-to-End Test 06: Function Calls, Recursion & Register Preserving
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>

int64_t factorial_recursive(int64_t n) {
    if (n <= 1) return 1;
    return n * factorial_recursive(n - 1);
}

int64_t fibonacci_recursive(int64_t n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return fibonacci_recursive(n - 1) + fibonacci_recursive(n - 2);
}

int64_t multi_arg_sum(int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6) {
    int64_t s1 = a1 + a2;
    int64_t s2 = a3 + a4;
    int64_t s3 = a5 + a6;
    return (s1 * s2) + s3;
}

