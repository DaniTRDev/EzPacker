#ifndef ABI_TEST_COMMON_H
#define ABI_TEST_COMMON_H

#if defined(ABI_SYSV)
#define ABI_ATTR __attribute__((sysv_abi))
#elif defined(ABI_WIN64)
#define ABI_ATTR __attribute__((ms_abi))
#else
#define ABI_ATTR
#endif

#endif // ABI_TEST_COMMON_H
