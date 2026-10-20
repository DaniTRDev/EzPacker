// EzPacker YARP End-to-End Test 03: Memory Access & Array Indexing
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>
#include <stddef.h>

int64_t sum_array_elements(const int64_t *arr, size_t count) {
    if (arr == 0 || count == 0) return 0;
    int64_t total = 0;
    for (size_t i = 0; i < count; i = i + 1) {
        total = total + arr[i];
    }
    return total;
}

void reverse_buffer_inplace(uint32_t *buf, size_t len) {
    if (buf == 0 || len <= 1) return;
    size_t i = 0;
    size_t j = len - 1;
    while (i < j) {
        uint32_t temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
        i = i + 1;
        j = j - 1;
    }
}

int32_t dot_product(const int16_t *a, const int16_t *b, size_t n) {
    if (a == 0 || b == 0) return 0;
    int32_t acc = 0;
    for (size_t i = 0; i < n; i = i + 1) {
        acc = acc + ((int32_t)a[i] * (int32_t)b[i]);
    }
    return acc;
}

