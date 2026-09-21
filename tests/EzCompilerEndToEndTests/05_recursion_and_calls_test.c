#include <stdio.h>
#include <stdint.h>
#include <assert.h>

extern int64_t factorial(int64_t n);
extern int64_t fibonacci(int64_t n);

int main(void) {
    printf("[E2E Test 05] Running recursion and calls tests...\n");

    // 1. factorial
    assert(factorial(0) == 1);
    assert(factorial(1) == 1);
    assert(factorial(3) == 6);
    assert(factorial(5) == 120);
    assert(factorial(10) == 3628800);
    assert(factorial(12) == 479001600);

    // 2. fibonacci
    assert(fibonacci(0) == 0);
    assert(fibonacci(1) == 1);
    assert(fibonacci(2) == 1);
    assert(fibonacci(3) == 2);
    assert(fibonacci(6) == 8);
    assert(fibonacci(10) == 55);
    assert(fibonacci(12) == 144);

    printf("[E2E Test 05] PASS: All recursion and calls tests succeeded.\n");
    return 0;
}
