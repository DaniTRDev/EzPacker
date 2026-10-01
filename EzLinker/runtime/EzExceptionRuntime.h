#ifndef EZEXCEPTION_RUNTIME_H
#define EZEXCEPTION_RUNTIME_H

#include <setjmp.h>
#include <cstdint>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(_WIN32)
  #if defined(EZEXCEPTION_RUNTIME_EXPORTS)
    #define EZ_EX_API __declspec(dllexport)
  #else
    #define EZ_EX_API
  #endif
#else
  #define EZ_EX_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thread-local exception context frame containing SjLj jump buffer and unwind links.
 */
struct EzExceptionFrame
{
    jmp_buf jmpBuf;                  ///< Standard C setjmp buffer.
    struct EzExceptionFrame *prev;   ///< Link to enclosing parent exception frame.
    void *currentPayload;            ///< Exception value or RichExceptionPayload pointer.
    const void *currentRtti;         ///< Type descriptor pointer (or nullptr when --no-rtti).
    int isCaught;                    ///< Flag indicating if exception was matched and handled.
};

/**
 * Registers an exception frame on the thread-local exception stack.
 * Returns 0 on initial setup.
 */
EZ_EX_API int __ez_try_enter(struct EzExceptionFrame *frame);

/**
 * Unregisters an exception frame upon normal exit of a try or catch block.
 */
EZ_EX_API void __ez_try_leave(struct EzExceptionFrame *frame);

/**
 * Raises an exception with the given payload and RTTI descriptor, unwinding
 * to the nearest active try frame via longjmp.
 */
EZ_EX_API void __ez_throw(void *payload, const void *rtti);

/**
 * Evaluates whether a thrown RTTI matches the catch filter type descriptor.
 * Returns 1 if matches (or if filter is null for catch-all), 0 otherwise.
 */
EZ_EX_API int __ez_catch_matches(const void *thrownRtti, const void *filterRtti);

/**
 * Retrieves the payload of the most recently thrown exception on the current thread.
 */
EZ_EX_API void *__ez_get_current_exception(void);

/**
 * Retrieves the RTTI descriptor of the most recently thrown exception on the current thread.
 */
EZ_EX_API const void *__ez_get_current_rtti(void);

/**
 * Retrieves the active top-of-stack exception frame on the current thread.
 */
EZ_EX_API struct EzExceptionFrame *__ez_get_top_frame(void);

/**
 * Resets the thread-local exception state. Provided for unit testing and test isolation.
 */
EZ_EX_API void __ez_runtime_reset_for_testing(void);

#ifdef __cplusplus
}
#endif

#endif // EZEXCEPTION_RUNTIME_H
