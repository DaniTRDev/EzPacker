#ifndef EZDSLSEMA_SYMBOL_COMMON_H
#define EZDSLSEMA_SYMBOL_COMMON_H

#include "EzDslSemaCommon.h"

using SymbolId = size_t;                                // Index of a Symbol within the SymbolTable's symbol arena.
inline constexpr SymbolId InvalidSymbolId = UINT64_MAX; // Sentinel for "no symbol".

#endif // EZDSLSEMA_SYMBOL_COMMON_H