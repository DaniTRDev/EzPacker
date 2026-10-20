// EzPacker YARP End-to-End Test 01: Scalar Arithmetic & Bitwise Logic
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>

int64_t compute_arith_64(int64_t a, int64_t b, int64_t c) {
    int64_t sum = a + b;
    int64_t prod = sum * c;
    int64_t diff = prod - a;
    return diff;
}

int32_t compute_arith_32(int32_t a, int32_t b, int32_t c) {
    int32_t res = (a * b) + (b - c) * 3;
    return res;
}

uint64_t compute_bitwise_chain(uint64_t a, uint64_t b, uint64_t c) {
    uint64_t x = (a & b) | c;
    uint64_t y = x ^ (a << 3);
    uint64_t z = ~y + (b >> 2);
    return z;
}

int64_t compute_div_mod(int64_t a, int64_t b) {
    if (b == 0) return 0;
    int64_t q = a / b;
    int64_t r = a % b;
    return q + r;
}

