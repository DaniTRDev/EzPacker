#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

ABI_ATTR extern int64_t call_unary(int64_t a);
ABI_ATTR extern int64_t call_binary(int64_t a, int64_t b);
ABI_ATTR extern int64_t call_ternary(int64_t a, int64_t b, int64_t c);
ABI_ATTR extern int64_t call_quaternary(int64_t a, int64_t b, int64_t c, int64_t d);
ABI_ATTR extern int64_t call_composite(int64_t x, int64_t y);
ABI_ATTR extern int64_t call_5args(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e);
ABI_ATTR extern int64_t call_6args(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f);
ABI_ATTR extern int64_t call_7args(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g);
ABI_ATTR extern int64_t call_8args(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g, int64_t h);
ABI_ATTR extern int32_t call_8args_32(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h);

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

    // 6. call_5args: (a + b + c + d) - e
    // On Win64: 4 in regs, 1 on stack. On SysV: 5 in regs.
    assert(call_5args(1, 2, 3, 4, 5) == 5);
    assert(call_5args(10, 20, 30, 40, 50) == 50);
    assert(call_5args(100, 200, 300, 400, 1500) == -500);

    // 7. call_6args: (a + b + c + d + e) - f
    // On Win64: 4 in regs, 2 on stack. On SysV: 6 in regs.
    assert(call_6args(1, 2, 3, 4, 5, 6) == 9);
    assert(call_6args(10, 20, 30, 40, 50, 100) == 50);

    // 8. call_7args: (a + b + c + d + e + f) - g
    // On Win64: 4 in regs, 3 on stack. On SysV: 6 in regs, 1 on stack.
    assert(call_7args(1, 2, 3, 4, 5, 6, 7) == 14);
    assert(call_7args(10, 20, 30, 40, 50, 60, 200) == 10);

    // 9. call_8args: a + b + c + d + e + f + g + h
    // On Win64: 4 in regs, 4 on stack. On SysV: 6 in regs, 2 on stack.
    assert(call_8args(1, 2, 3, 4, 5, 6, 7, 8) == 36);
    assert(call_8args(10, 20, 30, 40, 50, 60, 70, 80) == 360);
    assert(call_8args(-1, -2, -3, -4, 5, 6, 7, 8) == 16);

    // 10. call_8args_32: 32-bit integer arguments across regs and stack
    assert(call_8args_32(1, 2, 3, 4, 5, 6, 7, 8) == 36);
    assert(call_8args_32(-10, 20, -30, 40, -50, 60, -70, 80) == 40);

    printf("[E2E Test 03] PASS: All calling convention tests succeeded.\n");
    return 0;
}
