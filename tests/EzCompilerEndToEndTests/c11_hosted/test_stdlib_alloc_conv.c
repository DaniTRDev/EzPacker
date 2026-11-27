#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

int main(void) {
    // free(NULL) must be a safe no-op
    free(NULL);

    // malloc(0) + realloc growth/shrink
    void *p0 = malloc(0);
    free(p0);

    int *arr = (int *)calloc(8, sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 8; i++) {
        assert(arr[i] == 0);
        arr[i] = (i + 1) * 10;
    }

    arr = (int *)realloc(arr, 16 * sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 8; i++) {
        assert(arr[i] == (i + 1) * 10);
    }
    arr[15] = 999;

    arr = (int *)realloc(arr, 4 * sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 4; i++) {
        assert(arr[i] == (i + 1) * 10);
    }
    free(arr);

    // strtoll / strtoull / strtol / atoi conversions
    char *endp = NULL;
    errno = 0;
    long long v1 = strtoll("  -1234567890123xyz", &endp, 10);
    assert(errno == 0);
    assert(v1 == -1234567890123LL);
    assert(strcmp(endp, "xyz") == 0);

    errno = 0;
    unsigned long long v2 = strtoull("0xdeadbeef", &endp, 0);
    assert(errno == 0);
    assert(v2 == 0xdeadbeefULL);
    assert(*endp == '\0');

    errno = 0;
    long long ov = strtoll("99999999999999999999999999999", &endp, 10);
    assert(errno == ERANGE);
    assert(ov == LLONG_MAX);

    assert(atoi("  +42") == 42);
    assert(abs(-77) == 77);
    assert(llabs(-900000000000LL) == 900000000000LL);

    div_t d = div(23, 5);
    assert(d.quot == 4 && d.rem == 3);

    div_t dn = div(-23, 5);
    assert(dn.quot == -4 && dn.rem == -3);

    assert(labs(-123456L) == 123456L);
    assert(strtol("0x2a", &endp, 0) == 42L);
    assert(*endp == '\0');

    puts("stdlib_alloc_conv ok");
    return 0;
}
