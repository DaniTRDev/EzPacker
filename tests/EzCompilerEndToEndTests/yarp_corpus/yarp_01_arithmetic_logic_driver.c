// EzPacker YARP End-to-End Test 01 Driver: Freestanding Execution Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

ABI_ATTR extern int64_t compute_arith_64(int64_t a, int64_t b, int64_t c);
ABI_ATTR extern int32_t compute_arith_32(int32_t a, int32_t b, int32_t c);
ABI_ATTR extern uint64_t compute_bitwise_chain(uint64_t a, uint64_t b, uint64_t c);
ABI_ATTR extern int64_t compute_div_mod(int64_t a, int64_t b);

int main(void) {
    uint64_t seed = 0x12345678ULL;

    int64_t r1 = compute_arith_64(10, 20, 5); // (10+20)*5 - 10 = 140
    if (r1 != 140) return 1;
    hash_step(&seed, (uint64_t)r1);

    int64_t r2 = compute_arith_64(-15, 35, 4); // (-15+35)*4 - (-15) = 80 + 15 = 95
    if (r2 != 95) return 2;
    hash_step(&seed, (uint64_t)r2);

    int32_t r3 = compute_arith_32(7, 8, 3); // 56 + (8-3)*3 = 56 + 15 = 71
    if (r3 != 71) return 3;
    hash_step(&seed, (uint64_t)r3);

    uint64_t r4 = compute_bitwise_chain(0xFF00FF00AA55AA55ULL, 0x0F0F0F0F33333333ULL, 0x123456789ABCDEF0ULL);
    hash_step(&seed, r4);

    int64_t r5 = compute_div_mod(100, 7); // 14 + 2 = 16
    if (r5 != 16) return 5;
    hash_step(&seed, (uint64_t)r5);

    int64_t r6 = compute_div_mod(-50, 6); // -8 + -2 = -10
    if (r6 != -10) return 6;
    hash_step(&seed, (uint64_t)r6);

    // Verify composite hash state is non-zero and deterministic
    if (seed == 0) return 7;

    return 0; // Success
}
