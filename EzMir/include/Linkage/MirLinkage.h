#ifndef EZMIR_MIR_LINKAGE_H
#define EZMIR_MIR_LINKAGE_H

#include <cstdint>

/**
 * Linkage visibility and resolution rules for symbols (functions and global variables)
 * across translation and compilation units.
 */
enum class MirLinkage : uint8_t
{
    External = 0, ///< Visible globally across translation units (standard external symbol).
    Internal,     ///< Private to this module / translation unit (static symbol).
    Weak          ///< Mergeable at link time; can be overridden by a non-weak definition.
};

/// Backward-compatible type alias for global variable linkage.
using MirGlobalVarLinkage = MirLinkage;

#endif // EZMIR_MIR_LINKAGE_H
