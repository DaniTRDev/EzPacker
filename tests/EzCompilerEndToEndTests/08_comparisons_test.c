#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

ABI_ATTR extern int32_t cmp_eq_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t cmp_ne_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t cmp_slt_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t cmp_sle_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t cmp_sgt_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t cmp_sge_32(int32_t a, int32_t b);

ABI_ATTR extern int32_t cmp_ult_32(uint32_t a, uint32_t b);
ABI_ATTR extern int32_t cmp_ule_32(uint32_t a, uint32_t b);
ABI_ATTR extern int32_t cmp_ugt_32(uint32_t a, uint32_t b);
ABI_ATTR extern int32_t cmp_uge_32(uint32_t a, uint32_t b);

ABI_ATTR extern int64_t cmp_slt_64(int64_t a, int64_t b);
ABI_ATTR extern int64_t cmp_ult_64(uint64_t a, uint64_t b);

ABI_ATTR extern int32_t min_32(int32_t a, int32_t b);
ABI_ATTR extern int64_t max_64(int64_t a, int64_t b);
ABI_ATTR extern int32_t abs_32(int32_t x);

int main(void) {
    printf("[E2E Test 08] Running comparison and branch tests...\n");

    // 1. Equality and Inequality
    assert(cmp_eq_32(100, 100) == 1);
    assert(cmp_eq_32(100, 200) == 0);
    assert(cmp_ne_32(100, 200) == 1);
    assert(cmp_ne_32(100, 100) == 0);

    // 2. Signed comparisons
    assert(cmp_slt_32(-10, 5) == 1);
    assert(cmp_slt_32(5, -10) == 0);
    assert(cmp_slt_32(5, 5) == 0);

    assert(cmp_sle_32(-10, -10) == 1);
    assert(cmp_sle_32(-10, -5) == 1);
    assert(cmp_sle_32(0, -1) == 0);

    assert(cmp_sgt_32(10, -5) == 1);
    assert(cmp_sgt_32(-5, 10) == 0);
    assert(cmp_sgt_32(10, 10) == 0);

    assert(cmp_sge_32(10, 10) == 1);
    assert(cmp_sge_32(10, 5) == 1);
    assert(cmp_sge_32(-1, 0) == 0);

    // 3. Unsigned comparisons
    // -1 as uint32_t is 0xFFFFFFFF, which is > 5 unsigned
    assert(cmp_ult_32(5u, (uint32_t)-1) == 1);
    assert(cmp_ult_32((uint32_t)-1, 5u) == 0);
    assert(cmp_ule_32(100u, 100u) == 1);
    assert(cmp_ugt_32((uint32_t)-1, 0u) == 1);
    assert(cmp_uge_32(500u, 500u) == 1);

    // 4. 64-bit comparisons
    assert(cmp_slt_64(-1000000000000LL, 1000000000000LL) == 1);
    assert(cmp_slt_64(1000000000000LL, -1000000000000LL) == 0);
    assert(cmp_ult_64(0uLL, (uint64_t)-1LL) == 1);

    // 5. Control flow using comparisons: min, max, abs
    assert(min_32(10, 20) == 10);
    assert(min_32(-50, 10) == -50);
    assert(min_32(42, 42) == 42);

    assert(max_64(100LL, 500LL) == 500LL);
    assert(max_64(-100LL, -500LL) == -100LL);
    assert(max_64(0LL, 0LL) == 0LL);

    assert(abs_32(42) == 42);
    assert(abs_32(-42) == 42);
    assert(abs_32(0) == 0);
    assert(abs_32(-9999) == 9999);

    printf("[E2E Test 08] PASS: All comparison and branch tests succeeded.\n");
    return 0;
}
