#ifndef EZTRIPLE_LIBCALL_SIGNATURE_H
#define EZTRIPLE_LIBCALL_SIGNATURE_H

#include "EzTripleCommon.h"
#include "Libcall/LibcallKind.h"
#include <vector>

/**
 * Portable representation of argument and return types for runtime function signatures.
 */
enum class LibcallTypeKind : uint8_t
{
    Void,
    I1,
    I8,
    I16,
    I32,
    I64,
    I128,
    F32,
    F64,
    F128,
    Ptr,
    SizeT
};

/**
 * Signature descriptor for a runtime library call.
 */
struct LibcallSignature
{
    LibcallTypeKind returnType{ LibcallTypeKind::Void };
    std::vector<LibcallTypeKind> paramTypes;
    bool isNoReturn{ false };
    bool isReadOnly{ false };

    bool operator==(const LibcallSignature &other) const = default;
};

/**
 * Returns the canonical C ABI signature for a standard LibcallKind.
 */
const LibcallSignature &getCanonicalSignature(LibcallKind kind);

#endif // EZTRIPLE_LIBCALL_SIGNATURE_H
