#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

ABI_ATTR extern int64_t add_sub_64(int64_t a, int64_t b, int64_t c);
ABI_ATTR extern int64_t mul_add_64(int64_t a, int64_t b, int64_t c);
ABI_ATTR extern int64_t bitwise_chain_64(int64_t a, int64_t b, int64_t c);
ABI_ATTR extern int32_t arithmetic_32(int32_t a, int32_t b, int32_t c);
ABI_ATTR extern int32_t bitwise_32(int32_t a, int32_t b);
ABI_ATTR extern int64_t sub_from_zero_64(int64_t a);

int main(void) {
    printf("[E2E Test 01] Running arithmetic and bitwise tests...\n");

    // 1. add_sub_64: (a + b) - c
    assert(add_sub_64(10, 20, 5) == 25);
    assert(add_sub_64(-10, 20, 5) == 5);
    assert(add_sub_64(1000000000LL, 2000000000LL, 500000000LL) == 2500000000LL);
    assert(add_sub_64(0, 0, 0) == 0);

    // 2. mul_add_64: (a * b) + c
    assert(mul_add_64(7, 8, 9) == 65);
    assert(mul_add_64(-3, 4, 10) == -2);
    assert(mul_add_64(0, 100, 42) == 42);

    // 3. bitwise_chain_64: ((a & b) | c) ^ a
    int64_t a = 0xAA55AA55AA55AA55LL;
    int64_t b = 0x0F0F0F0F0F0F0F0FLL;
    int64_t c = 0x0000FFFF0000FFFFLL;
    int64_t expected_chain = ((a & b) | c) ^ a;
    assert(bitwise_chain_64(a, b, c) == expected_chain);

    // 4. arithmetic_32: (a + b) * c
    assert(arithmetic_32(3, 4, 5) == 35);
    assert(arithmetic_32(-2, 5, 4) == 12);
    assert(arithmetic_32(100, -100, 999) == 0);

    // 5. bitwise_32: (a & b) | (a ^ b) == (a | b)
    assert(bitwise_32(0x1234, 0x5678) == (0x1234 | 0x5678));
    assert(bitwise_32(-1, 0) == -1);

    // 6. sub_from_zero_64: 0 - a
    assert(sub_from_zero_64(42) == -42);
    assert(sub_from_zero_64(-100) == 100);
    assert(sub_from_zero_64(0) == 0);

    printf("[E2E Test 01] PASS: All arithmetic and bitwise tests succeeded.\n");
    return 0;
}
