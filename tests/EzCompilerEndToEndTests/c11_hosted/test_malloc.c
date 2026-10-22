#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    printf("Before malloc\n");
    char *p = (char *)malloc(64);
    assert(p != NULL);
    p[0] = 42;
    p[63] = 99;
    assert(p[0] == 42);
    assert(p[63] == 99);
    printf("After malloc: %p\n", p);
    free(p);
    printf("After free\n");
    return 0;
}
