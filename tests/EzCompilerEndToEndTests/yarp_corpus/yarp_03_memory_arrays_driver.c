// EzPacker YARP End-to-End Test 03 Driver: Memory Access Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

ABI_ATTR extern int64_t sum_array_elements(const int64_t *arr, size_t count);
ABI_ATTR extern void reverse_buffer_inplace(uint32_t *buf, size_t len);
ABI_ATTR extern int32_t dot_product(const int16_t *a, const int16_t *b, size_t n);

int main(void) {
    uint64_t seed = 0xABCDEF01ULL;

    int64_t arr1[5] = { 10, -5, 20, -15, 30 };
    int64_t s1 = sum_array_elements(arr1, 5); // 10 - 5 + 20 - 15 + 30 = 40
    if (s1 != 40) return 1;
    hash_step(&seed, (uint64_t)s1);

    uint32_t buf[6] = { 1, 2, 3, 4, 5, 6 };
    reverse_buffer_inplace(buf, 6);
    if (buf[0] != 6 || buf[1] != 5 || buf[2] != 4 ||
        buf[3] != 3 || buf[4] != 2 || buf[5] != 1) {
        return 2;
    }
    for (size_t i = 0; i < 6; i = i + 1) {
        hash_step(&seed, buf[i]);
    }

    int16_t vec_a[4] = { 2, -3, 4, -5 };
    int16_t vec_b[4] = { 10, 20, 30, 40 };
    // 20 - 60 + 120 - 200 = -120
    int32_t dp = dot_product(vec_a, vec_b, 4);
    if (dp != -120) return 3;
    hash_step(&seed, (uint64_t)dp);

    if (seed == 0) return 4;
    return 0; // Success
}
