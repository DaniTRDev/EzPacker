#include <stdio.h>
#include <assert.h>

int main(void) {
    int arr[] = { 10, 20, 30 };
    printf("arr[0]=%d, arr[1]=%d, arr[2]=%d\n", arr[0], arr[1], arr[2]);
    assert(arr[0] == 10);
    assert(arr[1] == 20);
    assert(arr[2] == 30);
    return 0;
}
