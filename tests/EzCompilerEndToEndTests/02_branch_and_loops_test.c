#include <stdio.h>
#include <stdint.h>
#include <assert.h>

extern int64_t max_of_two(int64_t a, int64_t b);
extern int64_t clamp_range(int64_t val, int64_t min_val, int64_t max_val);
extern int64_t sum_loop(int64_t n);
extern int64_t power_of_two(int64_t exp);

int main(void) {
    printf("[E2E Test 02] Running branch and loop tests...\n");

    // 1. max_of_two
    assert(max_of_two(10, 20) == 20);
    assert(max_of_two(50, -50) == 50);
    assert(max_of_two(-100, -200) == -100);
    assert(max_of_two(7, 7) == 7);

    // 2. clamp_range
    assert(clamp_range(5, 10, 20) == 10);
    assert(clamp_range(25, 10, 20) == 20);
    assert(clamp_range(15, 10, 20) == 15);
    assert(clamp_range(10, 10, 20) == 10);
    assert(clamp_range(20, 10, 20) == 20);

    // 3. sum_loop: 1 + 2 + ... + n
    assert(sum_loop(0) == 0);
    assert(sum_loop(1) == 1);
    assert(sum_loop(5) == 15);
    assert(sum_loop(10) == 55);
    assert(sum_loop(100) == 5050);

    // 4. power_of_two: 2^exp
    assert(power_of_two(0) == 1);
    assert(power_of_two(1) == 2);
    assert(power_of_two(4) == 16);
    assert(power_of_two(10) == 1024);
    assert(power_of_two(16) == 65536);

    printf("[E2E Test 02] PASS: All branch and loop tests succeeded.\n");
    return 0;
}
