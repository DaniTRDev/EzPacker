#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

ABI_ATTR extern int64_t abs_difference(int64_t a, int64_t b);
ABI_ATTR extern int64_t multi_phi_diamond(int64_t cond_val, int64_t x, int64_t y);

int main(void) {
    printf("[E2E Test 06] Running PHI and control flow tests...\n");

    // 1. abs_difference
    assert(abs_difference(10, 4) == 6);
    assert(abs_difference(4, 10) == 6);
    assert(abs_difference(-5, -20) == 15);
    assert(abs_difference(-20, -5) == 15);
    assert(abs_difference(42, 42) == 0);

    // 2. multi_phi_diamond
    // cond > 0 -> (x + 10) + (y + 20) = x + y + 30
    assert(multi_phi_diamond(1, 10, 5) == (10 + 10 + 5 + 20));
    assert(multi_phi_diamond(100, 0, 0) == 30);

    // cond <= 0 -> (x - 5) + (y - 15) = x + y - 20
    assert(multi_phi_diamond(0, 10, 5) == (10 - 5 + 5 - 15));
    assert(multi_phi_diamond(-1, 50, 50) == (50 - 5 + 50 - 15));

    printf("[E2E Test 06] PASS: All PHI and control flow tests succeeded.\n");
    return 0;
}
