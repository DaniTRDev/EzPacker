#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    int id;
    char name[32];
    int score;
} Item;

static int compare_items(const void *a, const void *b) {
    const Item *ia = (const Item *)a;
    const Item *ib = (const Item *)b;
    return ib->score - ia->score; // Descending
}

int main(void) {
    Item items[3];
    items[0].id = 1;
    strncpy(items[0].name, "Alpha", 31);
    items[0].score = 75;

    items[1].id = 2;
    strncpy(items[1].name, "Beta", 31);
    items[1].score = 95;

    items[2].id = 3;
    strncpy(items[2].name, "Gamma", 31);
    items[2].score = 85;

    printf("Before sort: [0]=%s(%d), ", items[0].name, items[0].score);
    printf("[1]=%s(%d), ", items[1].name, items[1].score);
    printf("[2]=%s(%d)\n", items[2].name, items[2].score);

    qsort(items, 3, sizeof(Item), compare_items);

    printf("After sort: [0]=%s(%d), ", items[0].name, items[0].score);
    printf("[1]=%s(%d), ", items[1].name, items[1].score);
    printf("[2]=%s(%d)\n", items[2].name, items[2].score);

    assert(items[0].score == 95);
    assert(strcmp(items[0].name, "Beta") == 0);
    assert(items[1].score == 85);
    assert(strcmp(items[1].name, "Gamma") == 0);
    assert(items[2].score == 75);
    assert(strcmp(items[2].name, "Alpha") == 0);
    return 0;
}
