// EzPacker YARP End-to-End Test 02: Branching, Loops & Control Flow
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>
#include <stdbool.h>

#define MIN_VAL(a, b) (((a) < (b)) ? (a) : (b))
#define MAX_VAL(a, b) (((a) > (b)) ? (a) : (b))

int64_t compute_loop_accumulator(int64_t n, int64_t step) {
    int64_t sum = 0;
    for (int64_t i = 0; i < n; i = i + 1) {
        if (i % 2 == 0) {
            sum = sum + (i * step);
        } else {
            sum = sum - (i + step);
        }
    }
    return sum;
}

int32_t compute_collatz_steps(int32_t val) {
    if (val <= 0) return 0;
    for (int32_t steps = 0; steps < 1000; steps = steps + 1) {
        if (val == 1) return steps;
        if ((val & 1) == 0) {
            val = val >> 1;
        } else {
            val = 3 * val + 1;
        }
    }
    return 1000;
}

int64_t compute_clamped_minmax(int64_t val, int64_t low, int64_t high) {
    int64_t clamped = MAX_VAL(low, MIN_VAL(val, high));
    return clamped;
}

int32_t compute_nested_break(int32_t limit) {
    int32_t count = 0;
    for (int32_t i = 0; i < 20; i = i + 1) {
        for (int32_t j = 0; j < 20; j = j + 1) {
            if (i * j > limit) break;
            count = count + 1;
        }
    }
    return count;
}
