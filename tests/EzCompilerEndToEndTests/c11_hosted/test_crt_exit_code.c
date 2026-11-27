#include <stdio.h>
#include <stdlib.h>

static void cleanup_runs_on_exit(void) {
    puts("exit_code_atexit_ran");
    fflush(stdout);
}

static void trigger_exit(void) {
    exit(42);
}

int main(void) {
    if (atexit(cleanup_runs_on_exit) != 0) {
        return 1;
    }
    trigger_exit();
    return 0;
}
