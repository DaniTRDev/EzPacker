// EzPacker YARP End-to-End Test 02 Driver: Branching & Loops Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

ABI_ATTR extern int64_t compute_loop_accumulator(int64_t n, int64_t step);
ABI_ATTR extern int32_t compute_collatz_steps(int32_t n);
ABI_ATTR extern int64_t compute_clamped_minmax(int64_t val, int64_t low, int64_t high);
ABI_ATTR extern int32_t compute_nested_break(int32_t limit);

int main(void) {
    uint64_t seed = 0x87654321ULL;

    int64_t r1 = compute_loop_accumulator(10, 3);
    hash_step(&seed, (uint64_t)r1);

    int32_t r2 = compute_collatz_steps(27); // Standard Collatz of 27 is 111 steps
    if (r2 != 111) return 1;
    hash_step(&seed, (uint64_t)r2);

    int64_t r3 = compute_clamped_minmax(150, 10, 100); // Clamped to 100
    if (r3 != 100) return 2;
    hash_step(&seed, (uint64_t)r3);

    int64_t r4 = compute_clamped_minmax(-20, 0, 50); // Clamped to 0
    if (r4 != 0) return 3;
    hash_step(&seed, (uint64_t)r4);

    int32_t r5 = compute_nested_break(50);
    hash_step(&seed, (uint64_t)r5);

    if (seed == 0) return 4;
    return 0; // Success
}
