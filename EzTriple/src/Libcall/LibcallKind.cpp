#include "Libcall/LibcallKind.h"

LibcallProvider getLibcallProvider(LibcallKind kind)
{
    switch (kind)
    {
        // Integer arithmetic & bit ops -> CompilerRt
        case LibcallKind::DivI32:
        case LibcallKind::UDivI32:
        case LibcallKind::RemI32:
        case LibcallKind::URemI32:
        case LibcallKind::DivI64:
        case LibcallKind::UDivI64:
        case LibcallKind::RemI64:
        case LibcallKind::URemI64:
        case LibcallKind::DivI128:
        case LibcallKind::UDivI128:
        case LibcallKind::RemI128:
        case LibcallKind::URemI128:
        case LibcallKind::MulI64:
        case LibcallKind::MulI128:
        case LibcallKind::ShlI64:
        case LibcallKind::ShlI128:
        case LibcallKind::LShrI64:
        case LibcallKind::LShrI128:
        case LibcallKind::AShrI64:
        case LibcallKind::AShrI128:
        case LibcallKind::PopcountI32:
        case LibcallKind::PopcountI64:
        case LibcallKind::PopcountI128:
        case LibcallKind::ClzI32:
        case LibcallKind::ClzI64:
        case LibcallKind::ClzI128:
        case LibcallKind::CtzI32:
        case LibcallKind::CtzI64:
        case LibcallKind::CtzI128:
        case LibcallKind::ParityI32:
        case LibcallKind::ParityI64:
        case LibcallKind::ParityI128:
        // Soft-float -> CompilerRt
        case LibcallKind::AddF32:
        case LibcallKind::SubF32:
        case LibcallKind::MulF32:
        case LibcallKind::DivF32:
        case LibcallKind::AddF64:
        case LibcallKind::SubF64:
        case LibcallKind::MulF64:
        case LibcallKind::DivF64:
        case LibcallKind::AddF128:
        case LibcallKind::SubF128:
        case LibcallKind::MulF128:
        case LibcallKind::DivF128:
        case LibcallKind::CmpEqF32:
        case LibcallKind::CmpLtF32:
        case LibcallKind::CmpLeF32:
        case LibcallKind::CmpGtF32:
        case LibcallKind::CmpGeF32:
        case LibcallKind::CmpNeF32:
        case LibcallKind::CmpUnordF32:
        case LibcallKind::CmpEqF64:
        case LibcallKind::CmpLtF64:
        case LibcallKind::CmpLeF64:
        case LibcallKind::CmpGtF64:
        case LibcallKind::CmpGeF64:
        case LibcallKind::CmpNeF64:
        case LibcallKind::CmpUnordF64:
        case LibcallKind::CmpEqF128:
        case LibcallKind::CmpLtF128:
        case LibcallKind::CmpLeF128:
        case LibcallKind::CmpGtF128:
        case LibcallKind::CmpGeF128:
        case LibcallKind::CmpNeF128:
        case LibcallKind::CmpUnordF128:
        case LibcallKind::Int32ToF32:
        case LibcallKind::Int32ToF64:
        case LibcallKind::Int64ToF32:
        case LibcallKind::Int64ToF64:
        case LibcallKind::Int128ToF32:
        case LibcallKind::Int128ToF64:
        case LibcallKind::UInt32ToF32:
        case LibcallKind::UInt32ToF64:
        case LibcallKind::UInt64ToF32:
        case LibcallKind::UInt64ToF64:
        case LibcallKind::UInt128ToF32:
        case LibcallKind::UInt128ToF64:
        case LibcallKind::F32ToInt32:
        case LibcallKind::F32ToInt64:
        case LibcallKind::F32ToInt128:
        case LibcallKind::F64ToInt32:
        case LibcallKind::F64ToInt64:
        case LibcallKind::F64ToInt128:
        case LibcallKind::F32ToUInt32:
        case LibcallKind::F32ToUInt64:
        case LibcallKind::F32ToUInt128:
        case LibcallKind::F64ToUInt32:
        case LibcallKind::F64ToUInt64:
        case LibcallKind::F64ToUInt128:
        case LibcallKind::F32ToF64:
        case LibcallKind::F64ToF32:
        case LibcallKind::F32ToF128:
        case LibcallKind::F64ToF128:
        case LibcallKind::F128ToF32:
        case LibcallKind::F128ToF64:
            return LibcallProvider::CompilerRt;

        // Memory, Math, Process -> CRuntime
        case LibcallKind::Memcpy:
        case LibcallKind::Memmove:
        case LibcallKind::Memset:
        case LibcallKind::Memcmp:
        case LibcallKind::Bzero:
        case LibcallKind::SqrtF32:
        case LibcallKind::SqrtF64:
        case LibcallKind::SinF32:
        case LibcallKind::SinF64:
        case LibcallKind::CosF32:
        case LibcallKind::CosF64:
        case LibcallKind::PowF32:
        case LibcallKind::PowF64:
        case LibcallKind::ExpF32:
        case LibcallKind::ExpF64:
        case LibcallKind::LogF32:
        case LibcallKind::LogF64:
        case LibcallKind::FloorF32:
        case LibcallKind::FloorF64:
        case LibcallKind::CeilF32:
        case LibcallKind::CeilF64:
        case LibcallKind::RoundF32:
        case LibcallKind::RoundF64:
        case LibcallKind::TruncF32:
        case LibcallKind::TruncF64:
        case LibcallKind::FmodF32:
        case LibcallKind::FmodF64:
        case LibcallKind::Abort:
        case LibcallKind::Exit:
            return LibcallProvider::CRuntime;

        // Target builtins
        case LibcallKind::StackProbe:
            return LibcallProvider::TargetBuiltin;

        default:
            return LibcallProvider::Custom;
    }
}

std::string_view getLibcallKindName(LibcallKind kind)
{
    switch (kind)
    {
        case LibcallKind::DivI32: return "DivI32";
        case LibcallKind::UDivI32: return "UDivI32";
        case LibcallKind::RemI32: return "RemI32";
        case LibcallKind::URemI32: return "URemI32";
        case LibcallKind::DivI64: return "DivI64";
        case LibcallKind::UDivI64: return "UDivI64";
        case LibcallKind::RemI64: return "RemI64";
        case LibcallKind::URemI64: return "URemI64";
        case LibcallKind::DivI128: return "DivI128";
        case LibcallKind::UDivI128: return "UDivI128";
        case LibcallKind::RemI128: return "RemI128";
        case LibcallKind::URemI128: return "URemI128";
        case LibcallKind::MulI64: return "MulI64";
        case LibcallKind::MulI128: return "MulI128";
        case LibcallKind::ShlI64: return "ShlI64";
        case LibcallKind::ShlI128: return "ShlI128";
        case LibcallKind::LShrI64: return "LShrI64";
        case LibcallKind::LShrI128: return "LShrI128";
        case LibcallKind::AShrI64: return "AShrI64";
        case LibcallKind::AShrI128: return "AShrI128";
        case LibcallKind::PopcountI32: return "PopcountI32";
        case LibcallKind::PopcountI64: return "PopcountI64";
        case LibcallKind::PopcountI128: return "PopcountI128";
        case LibcallKind::ClzI32: return "ClzI32";
        case LibcallKind::ClzI64: return "ClzI64";
        case LibcallKind::ClzI128: return "ClzI128";
        case LibcallKind::CtzI32: return "CtzI32";
        case LibcallKind::CtzI64: return "CtzI64";
        case LibcallKind::CtzI128: return "CtzI128";
        case LibcallKind::ParityI32: return "ParityI32";
        case LibcallKind::ParityI64: return "ParityI64";
        case LibcallKind::ParityI128: return "ParityI128";
        case LibcallKind::AddF32: return "AddF32";
        case LibcallKind::SubF32: return "SubF32";
        case LibcallKind::MulF32: return "MulF32";
        case LibcallKind::DivF32: return "DivF32";
        case LibcallKind::AddF64: return "AddF64";
        case LibcallKind::SubF64: return "SubF64";
        case LibcallKind::MulF64: return "MulF64";
        case LibcallKind::DivF64: return "DivF64";
        case LibcallKind::AddF128: return "AddF128";
        case LibcallKind::SubF128: return "SubF128";
        case LibcallKind::MulF128: return "MulF128";
        case LibcallKind::DivF128: return "DivF128";
        case LibcallKind::CmpEqF32: return "CmpEqF32";
        case LibcallKind::CmpLtF32: return "CmpLtF32";
        case LibcallKind::CmpLeF32: return "CmpLeF32";
        case LibcallKind::CmpGtF32: return "CmpGtF32";
        case LibcallKind::CmpGeF32: return "CmpGeF32";
        case LibcallKind::CmpNeF32: return "CmpNeF32";
        case LibcallKind::CmpUnordF32: return "CmpUnordF32";
        case LibcallKind::CmpEqF64: return "CmpEqF64";
        case LibcallKind::CmpLtF64: return "CmpLtF64";
        case LibcallKind::CmpLeF64: return "CmpLeF64";
        case LibcallKind::CmpGtF64: return "CmpGtF64";
        case LibcallKind::CmpGeF64: return "CmpGeF64";
        case LibcallKind::CmpNeF64: return "CmpNeF64";
        case LibcallKind::CmpUnordF64: return "CmpUnordF64";
        case LibcallKind::CmpEqF128: return "CmpEqF128";
        case LibcallKind::CmpLtF128: return "CmpLtF128";
        case LibcallKind::CmpLeF128: return "CmpLeF128";
        case LibcallKind::CmpGtF128: return "CmpGtF128";
        case LibcallKind::CmpGeF128: return "CmpGeF128";
        case LibcallKind::CmpNeF128: return "CmpNeF128";
        case LibcallKind::CmpUnordF128: return "CmpUnordF128";
        case LibcallKind::Int32ToF32: return "Int32ToF32";
        case LibcallKind::Int32ToF64: return "Int32ToF64";
        case LibcallKind::Int64ToF32: return "Int64ToF32";
        case LibcallKind::Int64ToF64: return "Int64ToF64";
        case LibcallKind::Int128ToF32: return "Int128ToF32";
        case LibcallKind::Int128ToF64: return "Int128ToF64";
        case LibcallKind::UInt32ToF32: return "UInt32ToF32";
        case LibcallKind::UInt32ToF64: return "UInt32ToF64";
        case LibcallKind::UInt64ToF32: return "UInt64ToF32";
        case LibcallKind::UInt64ToF64: return "UInt64ToF64";
        case LibcallKind::UInt128ToF32: return "UInt128ToF32";
        case LibcallKind::UInt128ToF64: return "UInt128ToF64";
        case LibcallKind::F32ToInt32: return "F32ToInt32";
        case LibcallKind::F32ToInt64: return "F32ToInt64";
        case LibcallKind::F32ToInt128: return "F32ToInt128";
        case LibcallKind::F64ToInt32: return "F64ToInt32";
        case LibcallKind::F64ToInt64: return "F64ToInt64";
        case LibcallKind::F64ToInt128: return "F64ToInt128";
        case LibcallKind::F32ToUInt32: return "F32ToUInt32";
        case LibcallKind::F32ToUInt64: return "F32ToUInt64";
        case LibcallKind::F32ToUInt128: return "F32ToUInt128";
        case LibcallKind::F64ToUInt32: return "F64ToUInt32";
        case LibcallKind::F64ToUInt64: return "F64ToUInt64";
        case LibcallKind::F64ToUInt128: return "F64ToUInt128";
        case LibcallKind::F32ToF64: return "F32ToF64";
        case LibcallKind::F64ToF32: return "F64ToF32";
        case LibcallKind::F32ToF128: return "F32ToF128";
        case LibcallKind::F64ToF128: return "F64ToF128";
        case LibcallKind::F128ToF32: return "F128ToF32";
        case LibcallKind::F128ToF64: return "F128ToF64";
        case LibcallKind::Memcpy: return "Memcpy";
        case LibcallKind::Memmove: return "Memmove";
        case LibcallKind::Memset: return "Memset";
        case LibcallKind::Memcmp: return "Memcmp";
        case LibcallKind::Bzero: return "Bzero";
        case LibcallKind::SqrtF32: return "SqrtF32";
        case LibcallKind::SqrtF64: return "SqrtF64";
        case LibcallKind::SinF32: return "SinF32";
        case LibcallKind::SinF64: return "SinF64";
        case LibcallKind::CosF32: return "CosF32";
        case LibcallKind::CosF64: return "CosF64";
        case LibcallKind::PowF32: return "PowF32";
        case LibcallKind::PowF64: return "PowF64";
        case LibcallKind::ExpF32: return "ExpF32";
        case LibcallKind::ExpF64: return "ExpF64";
        case LibcallKind::LogF32: return "LogF32";
        case LibcallKind::LogF64: return "LogF64";
        case LibcallKind::FloorF32: return "FloorF32";
        case LibcallKind::FloorF64: return "FloorF64";
        case LibcallKind::CeilF32: return "CeilF32";
        case LibcallKind::CeilF64: return "CeilF64";
        case LibcallKind::RoundF32: return "RoundF32";
        case LibcallKind::RoundF64: return "RoundF64";
        case LibcallKind::TruncF32: return "TruncF32";
        case LibcallKind::TruncF64: return "TruncF64";
        case LibcallKind::FmodF32: return "FmodF32";
        case LibcallKind::FmodF64: return "FmodF64";
        case LibcallKind::Abort: return "Abort";
        case LibcallKind::Exit: return "Exit";
        case LibcallKind::StackProbe: return "StackProbe";
        default: return "Unknown";
    }
}

std::string_view getDefaultLibcallName(LibcallKind kind, CrtFlavor flavor)
{
    // Special handling for MSVC naming
    if (flavor == CrtFlavor::Msvc)
    {
        switch (kind)
        {
            case LibcallKind::DivI64: return "_alldiv";
            case LibcallKind::UDivI64: return "_aulldiv";
            case LibcallKind::RemI64: return "_allrem";
            case LibcallKind::URemI64: return "_aullrem";
            case LibcallKind::MulI64: return "_allmul";
            case LibcallKind::ShlI64: return "_allshl";
            case LibcallKind::LShrI64: return "_aullshr";
            case LibcallKind::AShrI64: return "_allshr";
            case LibcallKind::StackProbe: return "__chkstk";
            default: break;
        }
    }

    switch (kind)
    {
        // 32-bit Integer Div/Rem
        case LibcallKind::DivI32: return "__divsi3";
        case LibcallKind::UDivI32: return "__udivsi3";
        case LibcallKind::RemI32: return "__modsi3";
        case LibcallKind::URemI32: return "__umodsi3";

        // 64-bit Integer Div/Rem & Mul
        case LibcallKind::DivI64: return "__divdi3";
        case LibcallKind::UDivI64: return "__udivdi3";
        case LibcallKind::RemI64: return "__moddi3";
        case LibcallKind::URemI64: return "__umoddi3";
        case LibcallKind::MulI64: return "__muldi3";

        // 128-bit Integer Div/Rem & Mul
        case LibcallKind::DivI128: return "__divti3";
        case LibcallKind::UDivI128: return "__udivti3";
        case LibcallKind::RemI128: return "__modti3";
        case LibcallKind::URemI128: return "__umodti3";
        case LibcallKind::MulI128: return "__multi3";

        // Shifts
        case LibcallKind::ShlI64: return "__ashldi3";
        case LibcallKind::ShlI128: return "__ashlti3";
        case LibcallKind::LShrI64: return "__lshrdi3";
        case LibcallKind::LShrI128: return "__lshrti3";
        case LibcallKind::AShrI64: return "__ashrdi3";
        case LibcallKind::AShrI128: return "__ashrti3";

        // Bitwise / Count
        case LibcallKind::PopcountI32: return "__popcountsi2";
        case LibcallKind::PopcountI64: return "__popcountdi2";
        case LibcallKind::PopcountI128: return "__popcountti2";
        case LibcallKind::ClzI32: return "__clzsi2";
        case LibcallKind::ClzI64: return "__clzdi2";
        case LibcallKind::ClzI128: return "__clzti2";
        case LibcallKind::CtzI32: return "__ctzsi2";
        case LibcallKind::CtzI64: return "__ctzdi2";
        case LibcallKind::CtzI128: return "__ctzti2";
        case LibcallKind::ParityI32: return "__paritysi2";
        case LibcallKind::ParityI64: return "__paritydi2";
        case LibcallKind::ParityI128: return "__parityti2";

        // Soft-Float Arithmetic
        case LibcallKind::AddF32: return "__addsf3";
        case LibcallKind::SubF32: return "__subsf3";
        case LibcallKind::MulF32: return "__mulsf3";
        case LibcallKind::DivF32: return "__divsf3";
        case LibcallKind::AddF64: return "__adddf3";
        case LibcallKind::SubF64: return "__subdf3";
        case LibcallKind::MulF64: return "__muldf3";
        case LibcallKind::DivF64: return "__divdf3";
        case LibcallKind::AddF128: return "__addtf3";
        case LibcallKind::SubF128: return "__subtf3";
        case LibcallKind::MulF128: return "__multf3";
        case LibcallKind::DivF128: return "__divtf3";

        // Soft-Float Comparisons
        case LibcallKind::CmpEqF32: return "__eqsf2";
        case LibcallKind::CmpLtF32: return "__ltsf2";
        case LibcallKind::CmpLeF32: return "__lesf2";
        case LibcallKind::CmpGtF32: return "__gtsf2";
        case LibcallKind::CmpGeF32: return "__gesf2";
        case LibcallKind::CmpNeF32: return "__nesf2";
        case LibcallKind::CmpUnordF32: return "__unordsf2";
        case LibcallKind::CmpEqF64: return "__eqdf2";
        case LibcallKind::CmpLtF64: return "__ltdf2";
        case LibcallKind::CmpLeF64: return "__ledf2";
        case LibcallKind::CmpGtF64: return "__gtdf2";
        case LibcallKind::CmpGeF64: return "__gedf2";
        case LibcallKind::CmpNeF64: return "__nedf2";
        case LibcallKind::CmpUnordF64: return "__unorddf2";
        case LibcallKind::CmpEqF128: return "__eqtf2";
        case LibcallKind::CmpLtF128: return "__lttf2";
        case LibcallKind::CmpLeF128: return "__letf2";
        case LibcallKind::CmpGtF128: return "__gttf2";
        case LibcallKind::CmpGeF128: return "__getf2";
        case LibcallKind::CmpNeF128: return "__netf2";
        case LibcallKind::CmpUnordF128: return "__unordtf2";

        // Conversions
        case LibcallKind::Int32ToF32: return "__floatsisf";
        case LibcallKind::Int32ToF64: return "__floatsidf";
        case LibcallKind::Int64ToF32: return "__floatdisf";
        case LibcallKind::Int64ToF64: return "__floatdidf";
        case LibcallKind::Int128ToF32: return "__floattisf";
        case LibcallKind::Int128ToF64: return "__floattidf";
        case LibcallKind::UInt32ToF32: return "__floatunsisf";
        case LibcallKind::UInt32ToF64: return "__floatunsidf";
        case LibcallKind::UInt64ToF32: return "__floatundisf";
        case LibcallKind::UInt64ToF64: return "__floatundidf";
        case LibcallKind::UInt128ToF32: return "__floatuntisf";
        case LibcallKind::UInt128ToF64: return "__floatuntidf";
        case LibcallKind::F32ToInt32: return "__fixsfsi";
        case LibcallKind::F32ToInt64: return "__fixsfdi";
        case LibcallKind::F32ToInt128: return "__fixsfti";
        case LibcallKind::F64ToInt32: return "__fixdfsi";
        case LibcallKind::F64ToInt64: return "__fixdfdi";
        case LibcallKind::F64ToInt128: return "__fixdfti";
        case LibcallKind::F32ToUInt32: return "__fixunssfsi";
        case LibcallKind::F32ToUInt64: return "__fixunssfdi";
        case LibcallKind::F32ToUInt128: return "__fixunssfti";
        case LibcallKind::F64ToUInt32: return "__fixunsdfsi";
        case LibcallKind::F64ToUInt64: return "__fixunsdfdi";
        case LibcallKind::F64ToUInt128: return "__fixunsdfti";
        case LibcallKind::F32ToF64: return "__extendsfdf2";
        case LibcallKind::F64ToF32: return "__truncdfsf2";
        case LibcallKind::F32ToF128: return "__extendsftf2";
        case LibcallKind::F64ToF128: return "__extenddftf2";
        case LibcallKind::F128ToF32: return "__trunctfsf2";
        case LibcallKind::F128ToF64: return "__trunctfdf2";

        // C-rt Memory
        case LibcallKind::Memcpy: return "memcpy";
        case LibcallKind::Memmove: return "memmove";
        case LibcallKind::Memset: return "memset";
        case LibcallKind::Memcmp: return "memcmp";
        case LibcallKind::Bzero: return "bzero";

        // C-rt Math
        case LibcallKind::SqrtF32: return "sqrtf";
        case LibcallKind::SqrtF64: return "sqrt";
        case LibcallKind::SinF32: return "sinf";
        case LibcallKind::SinF64: return "sin";
        case LibcallKind::CosF32: return "cosf";
        case LibcallKind::CosF64: return "cos";
        case LibcallKind::PowF32: return "powf";
        case LibcallKind::PowF64: return "pow";
        case LibcallKind::ExpF32: return "expf";
        case LibcallKind::ExpF64: return "exp";
        case LibcallKind::LogF32: return "logf";
        case LibcallKind::LogF64: return "log";
        case LibcallKind::FloorF32: return "floorf";
        case LibcallKind::FloorF64: return "floor";
        case LibcallKind::CeilF32: return "ceilf";
        case LibcallKind::CeilF64: return "ceil";
        case LibcallKind::RoundF32: return "roundf";
        case LibcallKind::RoundF64: return "round";
        case LibcallKind::TruncF32: return "truncf";
        case LibcallKind::TruncF64: return "trunc";
        case LibcallKind::FmodF32: return "fmodf";
        case LibcallKind::FmodF64: return "fmod";

        // C-rt Process
        case LibcallKind::Abort: return "abort";
        case LibcallKind::Exit: return "exit";

        // Stack Probe
        case LibcallKind::StackProbe: return (flavor == CrtFlavor::Gnu) ? "___chkstk_ms" : "__chkstk";

        default: return {};
    }
}
