#ifndef _EZPACKER_ASSERT_H
#define _EZPACKER_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#define assert(expr) \
    ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define static_assert _Static_assert
#endif

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_ASSERT_H
