#ifndef _EZMUSL_WIN32_FEATURES_H
#define _EZMUSL_WIN32_FEATURES_H

#include "../../../third_party/musl/include/features.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define weak
#define hidden
#define weak_alias(old, new)
#else
#define weak __attribute__((__weak__))
#define hidden
#ifdef EZ_MUSL_WEAK_HOOKS
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))
#else
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((__alias__(#old)))
#endif
#endif

#endif
