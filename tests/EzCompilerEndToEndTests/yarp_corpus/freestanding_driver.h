#ifndef FREESTANDING_DRIVER_H
#define FREESTANDING_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(ABI_SYSV)
#define ABI_ATTR __attribute__((sysv_abi))
#elif defined(ABI_WIN64)
#define ABI_ATTR __attribute__((ms_abi))
#else
#define ABI_ATTR
#endif

// Standalone 64-bit hashing primitive (zero standard library dependency)
static inline void hash_step(uint64_t *s, uint64_t v) {
    *s ^= v + 0x9e3779b97f4a7c15ULL + ((*s) << 6) + ((*s) >> 2);
}

#endif // FREESTANDING_DRIVER_H

