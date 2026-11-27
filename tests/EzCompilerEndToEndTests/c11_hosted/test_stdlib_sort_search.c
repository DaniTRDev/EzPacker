#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int cmp_int(const void *a, const void *b) {
    int va = *(const int *)a;
    int vb = *(const int *)b;
    return (va > vb) - (va < vb);
}

int main(void) {
    // 1. Standard unsorted array with duplicates
    int data[] = { 42, -5, 17, 42, 0, -100, 99, 17 };
    size_t n = sizeof(data) / sizeof(data[0]);
    qsort(data, n, sizeof(int), cmp_int);

    int expected[] = { -100, -5, 0, 17, 17, 42, 42, 99 };
    for (size_t i = 0; i < n; i++) {
        assert(data[i] == expected[i]);
    }

    // 2. bsearch hit and miss (5 arguments: tested on SysV ELF where 5 args fit in registers;
    //    Win64 5th-arg outgoing stack-slot lowering is a known pre-existing ezc backend limitation)
#if !defined(_WIN32)
    int key_hit = 17;
    int *found = (int *)bsearch(&key_hit, data, n, sizeof(int), cmp_int);
    assert(found != NULL);
    assert(*found == 17);

    int key_min = -100;
    assert(bsearch(&key_min, data, n, sizeof(int), cmp_int) == &data[0]);

    int key_miss = 55;
    assert(bsearch(&key_miss, data, n, sizeof(int), cmp_int) == NULL);
#endif

    // 3. Edge cases: 0 and 1 element
    int single = 7;
    qsort(&single, 1, sizeof(int), cmp_int);
    assert(single == 7);
    qsort(&single, 0, sizeof(int), cmp_int);
#if !defined(_WIN32)
    assert(bsearch(&single, &single, 0, sizeof(int), cmp_int) == NULL);
#endif

    puts("stdlib_sort_search ok");
    return 0;
}
