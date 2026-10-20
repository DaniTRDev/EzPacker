// EzPacker YARP End-to-End Test 04: Type Conversions, Sign Extensions & Casts
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>
#include <stdbool.h>

uint32_t zero_extend_u8_to_u32(uint8_t val) {
    uint32_t extended = (uint32_t)val;
    return extended + 0x1000U;
}

int32_t sign_extend_i8_to_i32(int8_t val) {
    int32_t extended = (int32_t)val;
    return extended * 2;
}

uint16_t truncate_u64_to_u16(uint64_t val) {
    uint16_t trunc = (uint16_t)val;
    return trunc ^ 0xAAAAU;
}

int64_t mixed_signedness_arith(uint32_t u, int32_t s) {
    int64_t sum = (int64_t)u + (int64_t)s;
    return sum;
}

uint64_t bitfield_simulation(uint32_t raw) {
    uint8_t f1 = (uint8_t)(raw & 0xFFU);
    uint16_t f2 = (uint16_t)((raw >> 8) & 0xFFFFU);
    uint8_t f3 = (uint8_t)((raw >> 24) & 0xFFU);
    return ((uint64_t)f1 << 32) | ((uint64_t)f2 << 16) | (uint64_t)f3;
}

