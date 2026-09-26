#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "abi_test_common.h"

// External global variables defined in 11_global_variables.mir
extern int8_t g_byte;
extern int16_t g_word;
extern int32_t g_dword;
extern int64_t g_qword;
extern const int32_t g_const_int;
extern int32_t g_bss_int;
extern int64_t g_bss_zero;
extern int32_t g_weak_val;

// MIR functions (getters)
ABI_ATTR extern int8_t get_g_byte(void);
ABI_ATTR extern int16_t get_g_word(void);
ABI_ATTR extern int32_t get_g_dword(void);
ABI_ATTR extern int64_t get_g_qword(void);
ABI_ATTR extern int32_t get_g_const_int(void);
ABI_ATTR extern int32_t get_g_bss_int(void);
ABI_ATTR extern int64_t get_g_bss_zero(void);
ABI_ATTR extern int32_t get_internal_counter(void);
ABI_ATTR extern int32_t get_g_weak_val(void);

// MIR functions (setters)
ABI_ATTR extern int8_t set_g_byte(int8_t val);
ABI_ATTR extern int16_t set_g_word(int16_t val);
ABI_ATTR extern int32_t set_g_dword(int32_t val);
ABI_ATTR extern int64_t set_g_qword(int64_t val);
ABI_ATTR extern int32_t set_g_bss_int(int32_t val);
ABI_ATTR extern int64_t set_g_bss_zero(int64_t val);
ABI_ATTR extern int32_t set_internal_counter(int32_t val);

// MIR functions (operations)
ABI_ATTR extern int32_t inc_internal_counter(void);
ABI_ATTR extern int32_t accumulate_into_bss(int32_t delta);
ABI_ATTR extern int64_t compute_cross_globals(int64_t scale);
ABI_ATTR extern int32_t reset_all_globals(void);

int main(void) {
    printf("[E2E Test 11] Running global variables tests...\n");

    // =========================================================================
    // 1. Initial values verification (C direct access vs MIR getter)
    // =========================================================================
    assert(g_byte == 42);
    assert(get_g_byte() == 42);

    assert(g_word == 1234);
    assert(get_g_word() == 1234);

    assert(g_dword == 100000);
    assert(get_g_dword() == 100000);

    assert(g_qword == 0x1122334455667788LL);
    assert(get_g_qword() == 0x1122334455667788LL);

    assert(g_const_int == 987654);
    assert(get_g_const_int() == 987654);

    // BSS uninitialized and zero-initialized scalars
    assert(g_bss_int == 0);
    assert(get_g_bss_int() == 0);

    assert(g_bss_zero == 0LL);
    assert(get_g_bss_zero() == 0LL);

    // Weak symbol initial value
    assert(g_weak_val == 777);
    assert(get_g_weak_val() == 777);

    // Internal global (encapsulated in MIR module, not exposed to C directly)
    assert(get_internal_counter() == 10);

    // =========================================================================
    // 2. Mutation from MIR, observed from C and MIR
    // =========================================================================
    set_g_byte(-10);
    assert(g_byte == -10);
    assert(get_g_byte() == -10);

    set_g_word((int16_t)0x55AA);
    assert(g_word == (int16_t)0x55AA);
    assert(get_g_word() == (int16_t)0x55AA);

    set_g_dword(-424242);
    assert(g_dword == -424242);
    assert(get_g_dword() == -424242);

    set_g_qword(-100000000000LL);
    assert(g_qword == -100000000000LL);
    assert(get_g_qword() == -100000000000LL);

    set_g_bss_int(123456);
    assert(g_bss_int == 123456);
    assert(get_g_bss_int() == 123456);

    set_g_bss_zero(0xDEADBEEFCAFEBABELL);
    assert(g_bss_zero == (int64_t)0xDEADBEEFCAFEBABELL);
    assert(get_g_bss_zero() == (int64_t)0xDEADBEEFCAFEBABELL);

    // =========================================================================
    // 3. Mutation from C, observed from MIR
    // =========================================================================
    g_byte = 100;
    assert(get_g_byte() == 100);

    g_word = -32000;
    assert(get_g_word() == -32000);

    g_dword = 888888;
    assert(get_g_dword() == 888888);

    g_qword = 0x0102030405060708LL;
    assert(get_g_qword() == 0x0102030405060708LL);

    g_bss_int = -1;
    assert(get_g_bss_int() == -1);

    g_bss_zero = -999999999LL;
    assert(get_g_bss_zero() == -999999999LL);

    // =========================================================================
    // 4. Internal global operations (counter increments)
    // =========================================================================
    set_internal_counter(50);
    assert(get_internal_counter() == 50);

    assert(inc_internal_counter() == 51);
    assert(inc_internal_counter() == 52);
    assert(inc_internal_counter() == 53);
    assert(get_internal_counter() == 53);

    // =========================================================================
    // 5. Complex operations: accumulate into BSS
    // =========================================================================
    set_g_bss_int(0);
    assert(accumulate_into_bss(10) == 10);
    assert(accumulate_into_bss(25) == 35);
    assert(accumulate_into_bss(-15) == 20);
    assert(g_bss_int == 20);

    // =========================================================================
    // 6. Cross-global computations: (g_qword * scale) + 100
    // =========================================================================
    g_qword = 50LL;
    // compute_cross_globals(3) -> 50 * 3 + 100 = 250
    assert(compute_cross_globals(3) == 250LL);

    g_qword = 1000LL;
    // compute_cross_globals(-2) -> 1000 * (-2) + 100 = -1900
    assert(compute_cross_globals(-2) == -1900LL);

    // =========================================================================
    // 7. Reset all globals
    // =========================================================================
    assert(reset_all_globals() == 0);
    assert(g_byte == 42);
    assert(g_word == 1234);
    assert(g_dword == 100000);
    assert(g_qword == 0x1122334455667788LL);
    assert(g_bss_int == 0);
    assert(g_bss_zero == 0LL);
    assert(get_internal_counter() == 10);

    printf("[E2E Test 11] All global variables tests PASSED!\n");
    return 0;
}
