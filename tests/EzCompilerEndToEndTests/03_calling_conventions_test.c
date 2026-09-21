#include <stdio.h>
#include <stdint.h>
#include <assert.h>

extern int64_t call_unary(int64_t a);
extern int64_t call_binary(int64_t a, int64_t b);
extern int64_t call_ternary(int64_t a, int64_t b, int64_t c);
extern int64_t call_quaternary(int64_t a, int64_t b, int64_t c, int64_t d);
extern int64_t call_composite(int64_t x, int64_t y);

int main(void) {
    printf("[E2E Test 03] Running calling convention tests...\n");

    // 1. call_unary: a + 10
    assert(call_unary(5) == 15);
    assert(call_unary(-10) == 0);
    assert(call_unary(990) == 1000);

    // 2. call_binary: (a + b) * (a - b) = a^2 - b^2
    assert(call_binary(5, 3) == 16);
    assert(call_binary(10, 10) == 0);
    assert(call_binary(4, 5) == -9);

    // 3. call_ternary: a + b + c
    assert(call_ternary(1, 2, 3) == 6);
    assert(call_ternary(100, 200, 300) == 600);
    assert(call_ternary(-10, 20, -10) == 0);

    // 4. call_quaternary: (a + b) * (c + d)
    assert(call_quaternary(1, 2, 3, 4) == 21);
    assert(call_quaternary(10, -5, 20, -10) == 50);

    // 5. call_composite: (x + 10) + (x^2 - y^2)
    assert(call_composite(5, 3) == (15 + 16));
    assert(call_composite(0, 0) == 10);
    assert(call_composite(10, 6) == (20 + (100 - 36)));

    printf("[E2E Test 03] PASS: All calling convention tests succeeded.\n");
    return 0;
}
