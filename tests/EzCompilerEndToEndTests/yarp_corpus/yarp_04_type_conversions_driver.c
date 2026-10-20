// EzPacker YARP End-to-End Test 04 Driver: Type Conversions Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

ABI_ATTR extern uint32_t zero_extend_u8_to_u32(uint8_t val);
ABI_ATTR extern int32_t sign_extend_i8_to_i32(int8_t val);
ABI_ATTR extern uint16_t truncate_u64_to_u16(uint64_t val);
ABI_ATTR extern int64_t mixed_signedness_arith(uint32_t u, int32_t s);
ABI_ATTR extern uint64_t bitfield_simulation(uint32_t raw);

int main(void) {
    uint64_t seed = 0x55AA55AAULL;

    uint32_t r1 = zero_extend_u8_to_u32(0xFE); // 0xFE + 0x1000 = 0x10FE
    if (r1 != 0x10FE) return 1;
    hash_step(&seed, (uint64_t)r1);

    int32_t r2 = sign_extend_i8_to_i32((int8_t)-50); // -50 * 2 = -100
    if (r2 != -100) return 2;
    hash_step(&seed, (uint64_t)r2);

    uint16_t r3 = truncate_u64_to_u16(0x123456789ABCDEF0ULL); // 0xDEF0 ^ 0xAAAA = 0x745A
    if (r3 != 0x745A) return 3;
    hash_step(&seed, (uint64_t)r3);

    int64_t r4 = mixed_signedness_arith(3000000000U, -1000000000); // 2000000000
    if (r4 != 2000000000LL) return 4;
    hash_step(&seed, (uint64_t)r4);

    uint64_t r5 = bitfield_simulation(0xAABBCCDD);
    hash_step(&seed, r5);

    if (seed == 0) return 5;
    return 0; // Success
}
