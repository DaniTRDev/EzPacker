#include "Libcall/LibcallSignature.h"
#include <unordered_map>

const LibcallSignature &getCanonicalSignature(LibcallKind kind)
{
    static const LibcallSignature s_defaultSig{};

    // Predefined signatures for all canonical kinds
    static const std::unordered_map<LibcallKind, LibcallSignature> s_signatures = []() {
        std::unordered_map<LibcallKind, LibcallSignature> map;

        // Binary 32-bit Integer Div/Rem: (i32, i32) -> i32
        LibcallSignature binI32{ .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I32, LibcallTypeKind::I32 } };
        map[LibcallKind::DivI32] = binI32;
        map[LibcallKind::UDivI32] = binI32;
        map[LibcallKind::RemI32] = binI32;
        map[LibcallKind::URemI32] = binI32;

        // Binary 64-bit Integer: (i64, i64) -> i64
        LibcallSignature binI64{ .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::I64, LibcallTypeKind::I64 } };
        map[LibcallKind::DivI64] = binI64;
        map[LibcallKind::UDivI64] = binI64;
        map[LibcallKind::RemI64] = binI64;
        map[LibcallKind::URemI64] = binI64;
        map[LibcallKind::MulI64] = binI64;

        // Binary 128-bit Integer: (i128, i128) -> i128
        LibcallSignature binI128{ .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::I128, LibcallTypeKind::I128 } };
        map[LibcallKind::DivI128] = binI128;
        map[LibcallKind::UDivI128] = binI128;
        map[LibcallKind::RemI128] = binI128;
        map[LibcallKind::URemI128] = binI128;
        map[LibcallKind::MulI128] = binI128;

        // Shifts: (val, i32) -> val
        map[LibcallKind::ShlI64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::I64, LibcallTypeKind::I32 } };
        map[LibcallKind::LShrI64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::I64, LibcallTypeKind::I32 } };
        map[LibcallKind::AShrI64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::I64, LibcallTypeKind::I32 } };
        map[LibcallKind::ShlI128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::I128, LibcallTypeKind::I32 } };
        map[LibcallKind::LShrI128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::I128, LibcallTypeKind::I32 } };
        map[LibcallKind::AShrI128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::I128, LibcallTypeKind::I32 } };

        // Bitwise counts: (val) -> i32
        map[LibcallKind::PopcountI32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::PopcountI64] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::PopcountI128] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I128 } };
        map[LibcallKind::ClzI32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::ClzI64] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::ClzI128] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I128 } };
        map[LibcallKind::CtzI32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::CtzI64] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::CtzI128] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I128 } };
        map[LibcallKind::ParityI32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::ParityI64] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::ParityI128] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::I128 } };

        // Binary Soft-Float
        LibcallSignature binF32{ .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::F32, LibcallTypeKind::F32 } };
        map[LibcallKind::AddF32] = binF32;
        map[LibcallKind::SubF32] = binF32;
        map[LibcallKind::MulF32] = binF32;
        map[LibcallKind::DivF32] = binF32;

        LibcallSignature binF64{ .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::F64, LibcallTypeKind::F64 } };
        map[LibcallKind::AddF64] = binF64;
        map[LibcallKind::SubF64] = binF64;
        map[LibcallKind::MulF64] = binF64;
        map[LibcallKind::DivF64] = binF64;

        LibcallSignature binF128{ .returnType = LibcallTypeKind::F128, .paramTypes = { LibcallTypeKind::F128, LibcallTypeKind::F128 } };
        map[LibcallKind::AddF128] = binF128;
        map[LibcallKind::SubF128] = binF128;
        map[LibcallKind::MulF128] = binF128;
        map[LibcallKind::DivF128] = binF128;

        // Float Comparisons: (F, F) -> i32
        LibcallSignature cmpF32{ .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F32, LibcallTypeKind::F32 } };
        map[LibcallKind::CmpEqF32] = cmpF32;
        map[LibcallKind::CmpLtF32] = cmpF32;
        map[LibcallKind::CmpLeF32] = cmpF32;
        map[LibcallKind::CmpGtF32] = cmpF32;
        map[LibcallKind::CmpGeF32] = cmpF32;
        map[LibcallKind::CmpNeF32] = cmpF32;
        map[LibcallKind::CmpUnordF32] = cmpF32;

        LibcallSignature cmpF64{ .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F64, LibcallTypeKind::F64 } };
        map[LibcallKind::CmpEqF64] = cmpF64;
        map[LibcallKind::CmpLtF64] = cmpF64;
        map[LibcallKind::CmpLeF64] = cmpF64;
        map[LibcallKind::CmpGtF64] = cmpF64;
        map[LibcallKind::CmpGeF64] = cmpF64;
        map[LibcallKind::CmpNeF64] = cmpF64;
        map[LibcallKind::CmpUnordF64] = cmpF64;

        LibcallSignature cmpF128{ .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F128, LibcallTypeKind::F128 } };
        map[LibcallKind::CmpEqF128] = cmpF128;
        map[LibcallKind::CmpLtF128] = cmpF128;
        map[LibcallKind::CmpLeF128] = cmpF128;
        map[LibcallKind::CmpGtF128] = cmpF128;
        map[LibcallKind::CmpGeF128] = cmpF128;
        map[LibcallKind::CmpNeF128] = cmpF128;
        map[LibcallKind::CmpUnordF128] = cmpF128;

        // Float Conversions
        map[LibcallKind::Int32ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::Int32ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::Int64ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::Int64ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::Int128ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I128 } };
        map[LibcallKind::Int128ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I128 } };

        map[LibcallKind::UInt32ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::UInt32ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I32 } };
        map[LibcallKind::UInt64ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::UInt64ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I64 } };
        map[LibcallKind::UInt128ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::I128 } };
        map[LibcallKind::UInt128ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::I128 } };

        map[LibcallKind::F32ToInt32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F32ToInt64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F32ToInt128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F64ToInt32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F64ToInt64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F64ToInt128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::F64 } };

        map[LibcallKind::F32ToUInt32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F32ToUInt64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F32ToUInt128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F64ToUInt32] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F64ToUInt64] = { .returnType = LibcallTypeKind::I64, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F64ToUInt128] = { .returnType = LibcallTypeKind::I128, .paramTypes = { LibcallTypeKind::F64 } };

        map[LibcallKind::F32ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F64ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F32ToF128] = { .returnType = LibcallTypeKind::F128, .paramTypes = { LibcallTypeKind::F32 } };
        map[LibcallKind::F64ToF128] = { .returnType = LibcallTypeKind::F128, .paramTypes = { LibcallTypeKind::F64 } };
        map[LibcallKind::F128ToF32] = { .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::F128 } };
        map[LibcallKind::F128ToF64] = { .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::F128 } };

        // C-rt Memory
        map[LibcallKind::Memcpy] = { .returnType = LibcallTypeKind::Ptr, .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::Ptr, LibcallTypeKind::SizeT } };
        map[LibcallKind::Memmove] = { .returnType = LibcallTypeKind::Ptr, .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::Ptr, LibcallTypeKind::SizeT } };
        map[LibcallKind::Memset] = { .returnType = LibcallTypeKind::Ptr, .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::I32, LibcallTypeKind::SizeT } };
        map[LibcallKind::Memcmp] = { .returnType = LibcallTypeKind::I32, .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::Ptr, LibcallTypeKind::SizeT }, .isReadOnly = true };
        map[LibcallKind::Bzero] = { .returnType = LibcallTypeKind::Void, .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::SizeT } };

        // C-rt Math (unary: F -> F)
        LibcallSignature unaryF32{ .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::F32 }, .isReadOnly = true };
        LibcallSignature unaryF64{ .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::F64 }, .isReadOnly = true };
        map[LibcallKind::SqrtF32] = unaryF32;
        map[LibcallKind::SqrtF64] = unaryF64;
        map[LibcallKind::SinF32] = unaryF32;
        map[LibcallKind::SinF64] = unaryF64;
        map[LibcallKind::CosF32] = unaryF32;
        map[LibcallKind::CosF64] = unaryF64;
        map[LibcallKind::ExpF32] = unaryF32;
        map[LibcallKind::ExpF64] = unaryF64;
        map[LibcallKind::LogF32] = unaryF32;
        map[LibcallKind::LogF64] = unaryF64;
        map[LibcallKind::FloorF32] = unaryF32;
        map[LibcallKind::FloorF64] = unaryF64;
        map[LibcallKind::CeilF32] = unaryF32;
        map[LibcallKind::CeilF64] = unaryF64;
        map[LibcallKind::RoundF32] = unaryF32;
        map[LibcallKind::RoundF64] = unaryF64;
        map[LibcallKind::TruncF32] = unaryF32;
        map[LibcallKind::TruncF64] = unaryF64;

        // C-rt Math (binary: (F, F) -> F)
        LibcallSignature mathBinF32{ .returnType = LibcallTypeKind::F32, .paramTypes = { LibcallTypeKind::F32, LibcallTypeKind::F32 }, .isReadOnly = true };
        LibcallSignature mathBinF64{ .returnType = LibcallTypeKind::F64, .paramTypes = { LibcallTypeKind::F64, LibcallTypeKind::F64 }, .isReadOnly = true };
        map[LibcallKind::PowF32] = mathBinF32;
        map[LibcallKind::PowF64] = mathBinF64;
        map[LibcallKind::FmodF32] = mathBinF32;
        map[LibcallKind::FmodF64] = mathBinF64;

        // C-rt Process
        map[LibcallKind::Abort] = { .returnType = LibcallTypeKind::Void, .paramTypes = {}, .isNoReturn = true };
        map[LibcallKind::Exit] = { .returnType = LibcallTypeKind::Void, .paramTypes = { LibcallTypeKind::I32 }, .isNoReturn = true };

        // Target builtins
        map[LibcallKind::StackProbe] = { .returnType = LibcallTypeKind::Void, .paramTypes = { LibcallTypeKind::SizeT } };

        return map;
    }();

    auto it = s_signatures.find(kind);
    if (it != s_signatures.end())
    {
        return it->second;
    }
    return s_defaultSig;
}
