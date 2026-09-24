#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "abi_test_common.h"

ABI_ATTR extern float add_f32(float a, float b);
ABI_ATTR extern float sub_f32(float a, float b);
ABI_ATTR extern float mul_f32(float a, float b);
ABI_ATTR extern float div_f32(float a, float b);

ABI_ATTR extern double add_f64(double a, double b);
ABI_ATTR extern double sub_f64(double a, double b);
ABI_ATTR extern double mul_f64(double a, double b);
ABI_ATTR extern double div_f64(double a, double b);

ABI_ATTR extern double poly_f64(double x, double a, double b, double c);

ABI_ATTR extern float int_to_float(int32_t x);
ABI_ATTR extern double int64_to_double(int64_t x);
ABI_ATTR extern int32_t float_to_int(float x);
ABI_ATTR extern int64_t double_to_int64(double x);

ABI_ATTR extern float mixed_args_add(int32_t a, float b, int32_t c, float d);

int main(void) {
    printf("[E2E Test 07] Running floating-point tests...\n");

    // 1. Single-precision basic arithmetic
    float s_add = add_f32(1.5f, 2.25f);
    assert(fabsf(s_add - 3.75f) < 1e-5f);

    float s_sub = sub_f32(5.5f, 2.25f);
    assert(fabsf(s_sub - 3.25f) < 1e-5f);

    float s_mul = mul_f32(3.0f, 4.5f);
    assert(fabsf(s_mul - 13.5f) < 1e-5f);

    float s_div = div_f32(15.0f, 3.0f);
    assert(fabsf(s_div - 5.0f) < 1e-5f);

    // 2. Double-precision basic arithmetic
    double d_add = add_f64(10.125, 20.375);
    assert(fabs(d_add - 30.5) < 1e-9);

    double d_sub = sub_f64(100.5, 42.25);
    assert(fabs(d_sub - 58.25) < 1e-9);

    double d_mul = mul_f64(2.5, 4.0);
    assert(fabs(d_mul - 10.0) < 1e-9);

    double d_div = div_f64(45.0, 9.0);
    assert(fabs(d_div - 5.0) < 1e-9);

    // 3. Polynomial evaluation: a*x^2 + b*x + c (4 FP arguments)
    // poly_f64(2.0, 3.0, 4.0, 5.0) = 3*(4) + 4*(2) + 5 = 12 + 8 + 5 = 25.0
    double poly_val = poly_f64(2.0, 3.0, 4.0, 5.0);
    assert(fabs(poly_val - 25.0) < 1e-9);

    // poly_f64(-1.5, 2.0, -3.0, 1.0) = 2*(2.25) - 3*(-1.5) + 1 = 4.5 + 4.5 + 1 = 10.0
    double poly_val2 = poly_f64(-1.5, 2.0, -3.0, 1.0);
    assert(fabs(poly_val2 - 10.0) < 1e-9);

    // 4. Integer to Float / Double conversions (SITOFP)
    float f_from_i = int_to_float(42);
    assert(fabsf(f_from_i - 42.0f) < 1e-5f);

    float f_from_neg = int_to_float(-100);
    assert(fabsf(f_from_neg - (-100.0f)) < 1e-5f);

    double d_from_i64 = int64_to_double(123456789012345LL);
    assert(fabs(d_from_i64 - 123456789012345.0) < 1.0);

    // 5. Float / Double to Integer conversions (FPTOSI - truncation toward zero)
    int32_t i_from_f = float_to_int(99.9f);
    assert(i_from_f == 99);

    int32_t i_from_neg_f = float_to_int(-15.75f);
    assert(i_from_neg_f == -15);

    int64_t i64_from_d = double_to_int64(9876543210.75);
    assert(i64_from_d == 9876543210LL);

    // 6. Mixed integer and float calling convention arguments
    // mixed_args_add(10, 2.5f, 20, 1.25f) = 10 + 2.5 + 20 + 1.25 = 33.75f
    float mixed_res = mixed_args_add(10, 2.5f, 20, 1.25f);
    assert(fabsf(mixed_res - 33.75f) < 1e-5f);

    float mixed_res2 = mixed_args_add(-5, 0.5f, 15, -2.5f);
    assert(fabsf(mixed_res2 - 8.0f) < 1e-5f);

    printf("[E2E Test 07] PASS: All floating-point tests succeeded.\n");
    return 0;
}
