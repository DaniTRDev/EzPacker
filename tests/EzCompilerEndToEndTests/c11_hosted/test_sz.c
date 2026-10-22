#include <stdio.h>
#include <assert.h>

struct Foo {
    char tag[16];
    int val;
};

int main(void) {
    struct Foo f;
    struct Foo *p = &f;
    printf("sizeof(f.tag)=%d, sizeof(p->tag)=%d\n", (int)sizeof(f.tag), (int)sizeof(p->tag));
    assert(sizeof(f.tag) == 16);
    assert(sizeof(p->tag) == 16);
    assert(sizeof(struct Foo) >= 20);
    return 0;
}
