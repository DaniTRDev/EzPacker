#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "abi_test_common.h"

typedef struct {
    int32_t x;
    int32_t y;
} Point2D;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} Vector3D;

ABI_ATTR extern int8_t mem_byte_roundtrip(void *buf, int8_t val);
ABI_ATTR extern int16_t mem_word_roundtrip(void *buf, int16_t val);
ABI_ATTR extern int32_t mem_dword_roundtrip(void *buf, int32_t val);
ABI_ATTR extern int64_t mem_qword_roundtrip(void *buf, int64_t val);

ABI_ATTR extern int32_t point2d_dist_sq(const Point2D *p1, const Point2D *p2);
ABI_ATTR extern int32_t point2d_translate(Point2D *p, int32_t dx, int32_t dy);
ABI_ATTR extern int32_t vec3_dot(const Vector3D *v1, const Vector3D *v2);
ABI_ATTR extern int32_t vec3_cross_z(const Vector3D *v1, const Vector3D *v2);

ABI_ATTR extern int32_t array4_sum_32(const int32_t *arr);
ABI_ATTR extern int32_t array4_reverse_32(int32_t *arr);

int main(void) {
    printf("[E2E Test 10] Running memory and aggregates tests...\n");

    // 1. Multi-width memory roundtrips
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));

    assert(mem_byte_roundtrip(buffer, (int8_t)0x5A) == (int8_t)0x5A);
    assert(mem_byte_roundtrip(buffer, (int8_t)-12) == (int8_t)-12);

    assert(mem_word_roundtrip(buffer, (int16_t)0x1234) == (int16_t)0x1234);
    assert(mem_word_roundtrip(buffer, (int16_t)-1000) == (int16_t)-1000);

    assert(mem_dword_roundtrip(buffer, 0x12345678) == 0x12345678);
    assert(mem_dword_roundtrip(buffer, -42) == -42);

    assert(mem_qword_roundtrip(buffer, 0x0123456789ABCDEFLL) == 0x0123456789ABCDEFLL);
    assert(mem_qword_roundtrip(buffer, -100000000000LL) == -100000000000LL);

    // 2. Point2D operations: dist_sq = (x1 - x2)^2 + (y1 - y2)^2
    Point2D p1 = { 0, 0 };
    Point2D p2 = { 3, 4 };
    assert(point2d_dist_sq(&p1, &p2) == 25); // 3^2 + 4^2 = 25

    Point2D p3 = { -2, -5 };
    Point2D p4 = { 1, -1 };
    assert(point2d_dist_sq(&p3, &p4) == 25); // (-3)^2 + (-4)^2 = 25

    // In-place translation
    Point2D pt = { 10, 20 };
    point2d_translate(&pt, 5, -8);
    assert(pt.x == 15);
    assert(pt.y == 12);

    // 3. Vector3D operations
    Vector3D v1 = { 1, 2, 3 };
    Vector3D v2 = { 4, -5, 6 };
    // dot = 1*4 + 2*(-5) + 3*6 = 4 - 10 + 18 = 12
    assert(vec3_dot(&v1, &v2) == 12);

    Vector3D v_unit_x = { 1, 0, 0 };
    Vector3D v_unit_y = { 0, 1, 0 };
    assert(vec3_dot(&v_unit_x, &v_unit_y) == 0); // orthogonal
    assert(vec3_cross_z(&v_unit_x, &v_unit_y) == 1); // x cross y has z = 1

    // 4. Fixed-size array operations
    int32_t arr[4] = { 10, 20, 30, 40 };
    assert(array4_sum_32(arr) == 100);

    array4_reverse_32(arr);
    assert(arr[0] == 40);
    assert(arr[1] == 30);
    assert(arr[2] == 20);
    assert(arr[3] == 10);
    assert(array4_sum_32(arr) == 100); // sum unchanged after reverse

    printf("[E2E Test 10] All memory and aggregates tests PASSED!\n");
    return 0;
}
