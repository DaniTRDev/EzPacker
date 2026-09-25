#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

ABI_ATTR extern int32_t bitwise_and_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_or_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_xor_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_not_32(int32_t a);
ABI_ATTR extern int32_t bitwise_nand_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_nor_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_xnor_32(int32_t a, int32_t b);
ABI_ATTR extern int32_t bitwise_mux_32(int32_t sel, int32_t a, int32_t b);
ABI_ATTR extern int32_t isolate_lowest_bit_32(int32_t x);
ABI_ATTR extern int32_t clear_lowest_bit_32(int32_t x);
ABI_ATTR extern int32_t is_power_of_two_32(int32_t x);
ABI_ATTR extern int32_t popcount_32(int32_t x);

ABI_ATTR extern int64_t bitwise_and_64(int64_t a, int64_t b);
ABI_ATTR extern int64_t bitwise_or_64(int64_t a, int64_t b);
ABI_ATTR extern int64_t bitwise_xor_64(int64_t a, int64_t b);
ABI_ATTR extern int64_t isolate_lowest_bit_64(int64_t x);
ABI_ATTR extern int64_t clear_lowest_bit_64(int64_t x);

int main(void) {
    printf("[E2E Test 09] Running bitwise logic and manipulation tests...\n");

    // 1. Basic 32-bit bitwise
    assert(bitwise_and_32(0xFF00FF00, 0x0F0F0F0F) == (int32_t)0x0F000F00);
    assert(bitwise_or_32(0xFF000000, 0x00FF0000) == (int32_t)0xFFFF0000);
    assert(bitwise_xor_32(0xAAAAAAAA, 0x55555555) == (int32_t)0xFFFFFFFF);
    assert(bitwise_not_32(0) == (int32_t)0xFFFFFFFF);
    assert(bitwise_not_32(0x12345678) == ~0x12345678);

    // 2. Logic gates (NAND, NOR, XNOR)
    assert(bitwise_nand_32(0xFFFFFFFF, 0x12345678) == ~0x12345678);
    assert(bitwise_nor_32(0x00000000, 0x12345678) == ~0x12345678);
    assert(bitwise_xnor_32(0x12345678, 0x12345678) == (int32_t)0xFFFFFFFF);
    assert(bitwise_xnor_32(0x12345678, ~0x12345678) == 0);

    // 3. Bitwise multiplexer: (sel & a) | (~sel & b)
    assert(bitwise_mux_32(0xFFFFFFFF, 0x11111111, 0x22222222) == 0x11111111);
    assert(bitwise_mux_32(0x00000000, 0x11111111, 0x22222222) == 0x22222222);
    assert(bitwise_mux_32(0x00FF00FF, 0x11111111, 0x22222222) == 0x22112211);

    // 4. Lowest set bit isolation: x & (-x)
    assert(isolate_lowest_bit_32(0) == 0);
    assert(isolate_lowest_bit_32(1) == 1);
    assert(isolate_lowest_bit_32(12) == 4);   // 1100 -> 0100 = 4
    assert(isolate_lowest_bit_32(40) == 8);   // 101000 -> 001000 = 8
    assert(isolate_lowest_bit_32(64) == 64);

    // 5. Lowest set bit clearing: x & (x - 1)
    assert(clear_lowest_bit_32(12) == 8);    // 1100 -> 1000 = 8
    assert(clear_lowest_bit_32(7) == 6);     // 0111 -> 0110 = 6
    assert(clear_lowest_bit_32(16) == 0);    // 10000 -> 00000 = 0

    // 6. Power of two check
    assert(is_power_of_two_32(0) == 0);
    assert(is_power_of_two_32(-4) == 0);
    assert(is_power_of_two_32(1) == 1);
    assert(is_power_of_two_32(2) == 1);
    assert(is_power_of_two_32(3) == 0);
    assert(is_power_of_two_32(4) == 1);
    assert(is_power_of_two_32(1024) == 1);
    assert(is_power_of_two_32(1025) == 0);

    // 7. Popcount (Brian Kernighan's loop)
    assert(popcount_32(0) == 0);
    assert(popcount_32(1) == 1);
    assert(popcount_32(2) == 1);
    assert(popcount_32(3) == 2);
    assert(popcount_32(7) == 3);
    assert(popcount_32(15) == 4);
    assert(popcount_32(0x55555555) == 16);
    assert(popcount_32(0xFFFFFFFF) == 32);

    // 8. 64-bit bitwise
    assert(bitwise_and_64(0x123456789ABCDEF0LL, 0x0F0F0F0F0F0F0F0FLL) == 0x020406080A0C0E00LL);
    assert(bitwise_or_64(0xF000000000000000LL, 0x000000000000000FLL) == (int64_t)0xF00000000000000FLL);
    assert(bitwise_xor_64(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL) == (int64_t)0xFFFFFFFFFFFFFFFFULL);
    assert(isolate_lowest_bit_64(0x100000000LL) == 0x100000000LL);
    assert(clear_lowest_bit_64(0x100000001LL) == 0x100000000LL);

    printf("[E2E Test 09] All bitwise logic and manipulation tests PASSED!\n");
    return 0;
}
