#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_exit_order = 0;

static void on_exit_first(void) {
    // Registered first, must run second (LIFO)
    assert(g_exit_order == 1);
    g_exit_order = 2;
    puts("atexit_second");
    fflush(stdout);
}

static void on_exit_second(void) {
    // Registered second, must run first (LIFO)
    assert(g_exit_order == 0);
    g_exit_order = 1;
    puts("atexit_first");
    fflush(stdout);
}

int main(int argc, char **argv) {
    assert(argc == 3);
    assert(argv != NULL);
    assert(argv[0] != NULL);
    assert(strcmp(argv[1], "alpha") == 0);
    assert(strcmp(argv[2], "beta") == 0);
    assert(argv[3] == NULL);

    assert(atexit(on_exit_first) == 0);
    assert(atexit(on_exit_second) == 0);

    printf("argc=%d arg1=%s arg2=%s\n", argc, argv[1], argv[2]);
    return 0;
}
