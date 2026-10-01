#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#ifndef __USE_MINGW_SETJMP_NON_SEH
#define __USE_MINGW_SETJMP_NON_SEH 1
#endif
#include <setjmp.h>

#include "abi_test_common.h"
#include "../../EzLinker/runtime/EzExceptionRuntime.h"

// Declarations of functions defined in 12_exceptions.mir
ABI_ATTR extern int64_t test_simple_throw_catch(int64_t val);
ABI_ATTR extern int64_t test_conditional_throw(int64_t balance, int64_t debit);

#if defined(ABI_SYSV) && defined(_WIN32)
#include <stdlib.h>
static void *s_sysv_payload = NULL;
static struct EzExceptionFrame *s_sysv_top_frame = NULL;

EZ_EX_API void __ez_throw(void *payload, const void *rtti) {
    (void)rtti;
    if (!s_sysv_top_frame) {
        fprintf(stderr, "fatal: uncaught SysV exception\n");
        abort();
    }
    struct EzExceptionFrame *target = s_sysv_top_frame;
    s_sysv_top_frame = target->prev;
    s_sysv_payload = payload;
    target->currentPayload = payload;
    target->isCaught = 1;
    longjmp(target->jmpBuf, 1);
}

EZ_EX_API void *__ez_get_current_exception(void) {
    return s_sysv_payload;
}

EZ_EX_API int __ez_try_enter(struct EzExceptionFrame *frame) {
    if (!frame) return 0;
    frame->prev = s_sysv_top_frame;
    frame->currentPayload = NULL;
    frame->isCaught = 0;
    s_sysv_top_frame = frame;
    return 0;
}

EZ_EX_API void __ez_try_leave(struct EzExceptionFrame *frame) {
    if (!frame) return;
    if (s_sysv_top_frame == frame) s_sysv_top_frame = frame->prev;
}

EZ_EX_API struct EzExceptionFrame *__ez_get_top_frame(void) {
    return s_sysv_top_frame;
}
#endif

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[E2E Test 12] Running exception handling tests...\n");

    // 1. test_simple_throw_catch: normal path (val >= 100) -> returns val * 2
    int64_t normal_res = test_simple_throw_catch(150);
    assert(normal_res == 300);
    printf("  [Pass] test_simple_throw_catch normal path: 150 * 2 = 300\n");

    // 2. test_simple_throw_catch: exceptional path (val < 100) -> throws 42, caught by handler
    {
        struct EzExceptionFrame frame;
        __ez_try_enter(&frame);
        if (setjmp(frame.jmpBuf) == 0) {
            test_simple_throw_catch(50);
            assert(0 && "Expected test_simple_throw_catch to throw!");
        } else {
            int64_t payload = (int64_t)(uintptr_t)__ez_get_current_exception();
            assert(payload == 42);
            printf("  [Pass] test_simple_throw_catch exception path: caught payload %lld\n", (long long)payload);
        }
        __ez_try_leave(&frame);
    }

    // 3. test_conditional_throw: normal path (balance >= debit) -> returns balance - debit
    int64_t debit_res = test_conditional_throw(500, 200);
    assert(debit_res == 300);
    printf("  [Pass] test_conditional_throw normal path: 500 - 200 = 300\n");

    // 4. test_conditional_throw: overdraft path (balance < debit) -> throws 402, caught by handler
    {
        struct EzExceptionFrame frame;
        __ez_try_enter(&frame);
        if (setjmp(frame.jmpBuf) == 0) {
            test_conditional_throw(100, 250);
            assert(0 && "Expected test_conditional_throw to throw overdraft!");
        } else {
            int64_t code = (int64_t)(uintptr_t)__ez_get_current_exception();
            assert(code == 402);
            printf("  [Pass] test_conditional_throw overdraft path: caught code %lld\n", (long long)code);
        }
        __ez_try_leave(&frame);
    }

    assert(__ez_get_top_frame() == NULL);

    printf("[E2E Test 12] PASS: All exception tests succeeded.\n");
    return 0;
}
