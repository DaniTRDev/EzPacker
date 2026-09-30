#ifndef EZTRIPLE_LIBCALL_KIND_H
#define EZTRIPLE_LIBCALL_KIND_H

#include "EzTripleCommon.h"
#include <string_view>

/**
 * Provider category classifying the origin and standard behavior of a libcall.
 */
enum class LibcallProvider : uint8_t
{
    CompilerRt,   ///< Low-level compiler helpers (e.g., __divti3, __multi3, soft-float).
    CRuntime,     ///< Standard C library functions (e.g., memcpy, memset, sin, abort).
    TargetBuiltin,///< Target-specific runtime builtins (e.g., __chkstk, AEABI helpers).
    Custom        ///< User-defined / ad-hoc runtime functions.
};

/**
 * Target C runtime flavor governing default symbol names and calling conventions.
 */
enum class CrtFlavor : uint8_t
{
    Gnu,      ///< GNU libc / libgcc / Linux / ELF standard.
    Msvc,     ///< Microsoft Visual C++ CRT (UCRT / MSVCRT).
    Musl,     ///< Musl libc.
    Darwin,   ///< Apple Darwin / macOS (Mach-O symbol prefixing conventions).
    BareMetal ///< Freestanding bare-metal environment (no hosted libc).
};

/**
 * Strongly-typed enumeration of standard runtime library functions supported across
 * compiler-rt and standard C-rt.
 */
enum class LibcallKind : uint16_t
{
    // --- Integer Arithmetic (compiler-rt) ---
    DivI32,
    UDivI32,
    RemI32,
    URemI32,

    DivI64,
    UDivI64,
    RemI64,
    URemI64,

    DivI128,
    UDivI128,
    RemI128,
    URemI128,

    MulI64,
    MulI128,

    // --- Integer Shifts (compiler-rt) ---
    ShlI64,
    ShlI128,
    LShrI64,
    LShrI128,
    AShrI64,
    AShrI128,

    // --- Bit Manipulation / Counting (compiler-rt) ---
    PopcountI32,
    PopcountI64,
    PopcountI128,
    ClzI32,
    ClzI64,
    ClzI128,
    CtzI32,
    CtzI64,
    CtzI128,
    ParityI32,
    ParityI64,
    ParityI128,

    // --- Soft-Float Arithmetic (compiler-rt) ---
    AddF32,
    SubF32,
    MulF32,
    DivF32,

    AddF64,
    SubF64,
    MulF64,
    DivF64,

    AddF128,
    SubF128,
    MulF128,
    DivF128,

    // --- Soft-Float Comparisons (compiler-rt) ---
    CmpEqF32,
    CmpLtF32,
    CmpLeF32,
    CmpGtF32,
    CmpGeF32,
    CmpNeF32,
    CmpUnordF32,

    CmpEqF64,
    CmpLtF64,
    CmpLeF64,
    CmpGtF64,
    CmpGeF64,
    CmpNeF64,
    CmpUnordF64,

    CmpEqF128,
    CmpLtF128,
    CmpLeF128,
    CmpGtF128,
    CmpGeF128,
    CmpNeF128,
    CmpUnordF128,

    // --- Soft-Float Conversions (compiler-rt) ---
    Int32ToF32,
    Int32ToF64,
    Int64ToF32,
    Int64ToF64,
    Int128ToF32,
    Int128ToF64,

    UInt32ToF32,
    UInt32ToF64,
    UInt64ToF32,
    UInt64ToF64,
    UInt128ToF32,
    UInt128ToF64,

    F32ToInt32,
    F32ToInt64,
    F32ToInt128,
    F64ToInt32,
    F64ToInt64,
    F64ToInt128,

    F32ToUInt32,
    F32ToUInt64,
    F32ToUInt128,
    F64ToUInt32,
    F64ToUInt64,
    F64ToUInt128,

    F32ToF64,
    F64ToF32,
    F32ToF128,
    F64ToF128,
    F128ToF32,
    F128ToF64,

    // --- Memory Operations (C-rt) ---
    Memcpy,
    Memmove,
    Memset,
    Memcmp,
    Bzero,

    // --- Math Operations (C-rt / libm) ---
    SqrtF32,
    SqrtF64,
    SinF32,
    SinF64,
    CosF32,
    CosF64,
    PowF32,
    PowF64,
    ExpF32,
    ExpF64,
    LogF32,
    LogF64,
    FloorF32,
    FloorF64,
    CeilF32,
    CeilF64,
    RoundF32,
    RoundF64,
    TruncF32,
    TruncF64,
    FmodF32,
    FmodF64,

    // --- Process / Environment (C-rt) ---
    Abort,
    Exit,

    // --- Stack Probing (Target builtins) ---
    StackProbe,

    COUNT
};

/**
 * Returns the provider category for a given libcall.
 */
LibcallProvider getLibcallProvider(LibcallKind kind);

/**
 * Returns true if the libcall is traditionally provided by compiler-rt / libgcc.
 */
inline bool isCompilerRt(LibcallKind kind)
{
    return getLibcallProvider(kind) == LibcallProvider::CompilerRt;
}

/**
 * Returns true if the libcall is provided by the standard C runtime (libc / libm).
 */
inline bool isCRuntime(LibcallKind kind)
{
    return getLibcallProvider(kind) == LibcallProvider::CRuntime;
}

/**
 * Returns the canonical symbolic name of the LibcallKind enum (e.g. "DivI128", "Memcpy").
 */
std::string_view getLibcallKindName(LibcallKind kind);

/**
 * Returns the default runtime symbol name for the given libcall and CRT flavor
 * (e.g., "__divti3" for DivI128, "memcpy" for Memcpy, "_alldiv" for DivI64 under MSVC 32-bit).
 */
std::string_view getDefaultLibcallName(LibcallKind kind, CrtFlavor flavor = CrtFlavor::Gnu);

#endif // EZTRIPLE_LIBCALL_KIND_H
