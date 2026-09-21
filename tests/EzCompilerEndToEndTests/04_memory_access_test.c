#include <stdio.h>
#include <stdint.h>
#include <assert.h>

extern int64_t mem_read_write(int64_t *buf, int64_t val);
extern int64_t mem_accumulate(const int64_t *buf);
extern int64_t mem_swap(int64_t *buf);
extern int64_t mem_scale_elements(int64_t *buf, int64_t factor);

int main(void) {
    printf("[E2E Test 04] Running memory access tests...\n");

    int64_t buffer[4] = { 0, 0, 0, 0 };

    // 1. mem_read_write
    int64_t result = mem_read_write(buffer, 123456789LL);
    assert(result == 123456789LL);
    assert(buffer[0] == 123456789LL);

    // 2. mem_accumulate
    int64_t tri_buf[3] = { 100, 250, 650 };
    int64_t total = mem_accumulate(tri_buf);
    assert(total == 1000);

    // 3. mem_swap
    int64_t pair[2] = { 42, 99 };
    mem_swap(pair);
    assert(pair[0] == 99);
    assert(pair[1] == 42);

    // 4. mem_scale_elements
    int64_t scale_buf[2] = { 7, -11 };
    mem_scale_elements(scale_buf, 3);
    assert(scale_buf[0] == 21);
    assert(scale_buf[1] == -33);

    printf("[E2E Test 04] PASS: All memory access tests succeeded.\n");
    return 0;
}
