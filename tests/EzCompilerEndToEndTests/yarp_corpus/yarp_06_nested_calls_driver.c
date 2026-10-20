// EzPacker YARP End-to-End Test 06 Driver: Function Calls & Recursion Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

ABI_ATTR extern int64_t factorial_recursive(int64_t n);
ABI_ATTR extern int64_t fibonacci_recursive(int64_t n);
ABI_ATTR extern int64_t multi_arg_sum(int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6);

int main(void) {
    uint64_t seed = 0x13579BDFULL;

    int64_t fact5 = factorial_recursive(5); // 120
    if (fact5 != 120) return 1;
    hash_step(&seed, (uint64_t)fact5);

    int64_t fact7 = factorial_recursive(7); // 5040
    if (fact7 != 5040) return 2;
    hash_step(&seed, (uint64_t)fact7);

    int64_t fib7 = fibonacci_recursive(7); // 13
    if (fib7 != 13) return 3;
    hash_step(&seed, (uint64_t)fib7);

    // (1+2)*(3+4) + (5+6) = 3*7 + 11 = 21 + 11 = 32
    int64_t msum = multi_arg_sum(1, 2, 3, 4, 5, 6);
    if (msum != 32) return 4;
    hash_step(&seed, (uint64_t)msum);

    if (seed == 0) return 5;
    return 0; // Success
}
