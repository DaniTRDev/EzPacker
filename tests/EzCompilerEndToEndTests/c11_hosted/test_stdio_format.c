#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    char buf[128];

    // Keep each variadic call within 4 arguments so Win64 and SysV both pass in registers
    int n1 = snprintf(buf, sizeof(buf), "int=%d", -42);
    assert(n1 == 7 && strcmp(buf, "int=-42") == 0);

    int n2 = snprintf(buf, sizeof(buf), "u=%u", 3000000000U);
    assert(n2 == 12 && strcmp(buf, "u=3000000000") == 0);

    int n3 = snprintf(buf, sizeof(buf), "hex=%x", 0xBEEF);
    assert(n3 == 8 && strcmp(buf, "hex=beef") == 0);

    int n4 = snprintf(buf, sizeof(buf), "str=%s", "musl");
    assert(n4 == 8 && strcmp(buf, "str=musl") == 0);

    int n5 = snprintf(buf, sizeof(buf), "ch=%c %%", 'Z');
    assert(n5 == 6 && strcmp(buf, "ch=Z %") == 0);

    // Truncation sizing: snprintf(NULL, 0, ...) must return full required length
    int req = snprintf(NULL, 0, "hello-%d", 2026);
    assert(req == 10);

    char small[6];
    int trunc_ret = snprintf(small, sizeof(small), "abcdefghij");
    assert(trunc_ret == 10);
    assert(strcmp(small, "abcde") == 0);

    // 64-bit integer, size_t, width, and zero-padding formatting
    int n6 = snprintf(buf, sizeof(buf), "ll=%lld", -9000000000LL);
    assert(n6 > 0 && strcmp(buf, "ll=-9000000000") == 0);

    int n7 = snprintf(buf, sizeof(buf), "ull=%llu", 18000000000ULL);
    assert(n7 > 0 && strcmp(buf, "ull=18000000000") == 0);

    int n8 = snprintf(buf, sizeof(buf), "zu=%zu", (size_t)64);
    assert(n8 > 0 && strcmp(buf, "zu=64") == 0);

    int n9 = snprintf(buf, sizeof(buf), "pad=%-6s", "ok");
    assert(n9 > 0 && strcmp(buf, "pad=ok    ") == 0);

    int n10 = snprintf(buf, sizeof(buf), "z=%04d", 7);
    assert(n10 > 0 && strcmp(buf, "z=0007") == 0);

    // sscanf parsing (<= 4 arguments per call)
    int a = 0;
    unsigned int b = 0;
    char word[16];
    memset(word, 0, sizeof(word));
    assert(sscanf("123 0x1a", "%d %x", &a, &b) == 2);
    assert(a == 123 && b == 0x1a);
    assert(sscanf("ezmusl", "%15s", word) == 1);
    assert(strcmp(word, "ezmusl") == 0);

    // Pointer formatting + stdout/fflush
    int dummy = 7;
    int np = snprintf(buf, sizeof(buf), "%p", (void *)&dummy);
    assert(np > 0);
    assert(fprintf(stdout, "stdio_format ok: %s\n", buf) > 0);
    assert(fflush(stdout) == 0);
    return 0;
}
