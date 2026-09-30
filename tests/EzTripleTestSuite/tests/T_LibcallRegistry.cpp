#include "EzTripleTestSuite.h"
#include "Libcall/LibcallKind.h"
#include "Libcall/LibcallSignature.h"
#include "Libcall/TargetLibcallRegistry.h"
#include "Type/MirTypeTable.h"

// Verifies provider classification and kind names for compiler-rt and C-rt functions.
TEST_F(EzTripleTestSuite, TestLibcallKindProviders)
{
    // Compiler-rt checks
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::UDivI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::MulI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ShlI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::PopcountI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::AddF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::AddF64));
    EXPECT_FALSE(isCRuntime(LibcallKind::DivI128));

    // C-rt checks
    EXPECT_TRUE(isCRuntime(LibcallKind::Memcpy));
    EXPECT_TRUE(isCRuntime(LibcallKind::Memmove));
    EXPECT_TRUE(isCRuntime(LibcallKind::Memset));
    EXPECT_TRUE(isCRuntime(LibcallKind::Memcmp));
    EXPECT_TRUE(isCRuntime(LibcallKind::SqrtF64));
    EXPECT_TRUE(isCRuntime(LibcallKind::SinF32));
    EXPECT_TRUE(isCRuntime(LibcallKind::PowF64));
    EXPECT_TRUE(isCRuntime(LibcallKind::Abort));
    EXPECT_TRUE(isCRuntime(LibcallKind::Exit));
    EXPECT_FALSE(isCompilerRt(LibcallKind::Memcpy));

    // Target builtins
    EXPECT_EQ(getLibcallProvider(LibcallKind::StackProbe), LibcallProvider::TargetBuiltin);

    // Kind names
    EXPECT_EQ(getLibcallKindName(LibcallKind::DivI128), "DivI128");
    EXPECT_EQ(getLibcallKindName(LibcallKind::Memcpy), "Memcpy");
    EXPECT_EQ(getLibcallKindName(LibcallKind::SqrtF64), "SqrtF64");
}

// Verifies canonical C ABI signatures across arithmetic, memory, and math libcalls.
TEST_F(EzTripleTestSuite, TestCanonicalSignatures)
{
    // DivI128: (i128, i128) -> i128
    const auto &divSig = getCanonicalSignature(LibcallKind::DivI128);
    EXPECT_EQ(divSig.returnType, LibcallTypeKind::I128);
    ASSERT_EQ(divSig.paramTypes.size(), 2u);
    EXPECT_EQ(divSig.paramTypes[0], LibcallTypeKind::I128);
    EXPECT_EQ(divSig.paramTypes[1], LibcallTypeKind::I128);
    EXPECT_FALSE(divSig.isNoReturn);

    // Memcpy: (ptr, ptr, size_t) -> ptr
    const auto &memcpySig = getCanonicalSignature(LibcallKind::Memcpy);
    EXPECT_EQ(memcpySig.returnType, LibcallTypeKind::Ptr);
    ASSERT_EQ(memcpySig.paramTypes.size(), 3u);
    EXPECT_EQ(memcpySig.paramTypes[0], LibcallTypeKind::Ptr);
    EXPECT_EQ(memcpySig.paramTypes[1], LibcallTypeKind::Ptr);
    EXPECT_EQ(memcpySig.paramTypes[2], LibcallTypeKind::SizeT);

    // Memset: (ptr, i32, size_t) -> ptr
    const auto &memsetSig = getCanonicalSignature(LibcallKind::Memset);
    EXPECT_EQ(memsetSig.returnType, LibcallTypeKind::Ptr);
    ASSERT_EQ(memsetSig.paramTypes.size(), 3u);
    EXPECT_EQ(memsetSig.paramTypes[0], LibcallTypeKind::Ptr);
    EXPECT_EQ(memsetSig.paramTypes[1], LibcallTypeKind::I32);
    EXPECT_EQ(memsetSig.paramTypes[2], LibcallTypeKind::SizeT);

    // SqrtF64: (f64) -> f64, read-only
    const auto &sqrtSig = getCanonicalSignature(LibcallKind::SqrtF64);
    EXPECT_EQ(sqrtSig.returnType, LibcallTypeKind::F64);
    ASSERT_EQ(sqrtSig.paramTypes.size(), 1u);
    EXPECT_EQ(sqrtSig.paramTypes[0], LibcallTypeKind::F64);
    EXPECT_TRUE(sqrtSig.isReadOnly);

    // Abort: () -> void, no-return
    const auto &abortSig = getCanonicalSignature(LibcallKind::Abort);
    EXPECT_EQ(abortSig.returnType, LibcallTypeKind::Void);
    EXPECT_TRUE(abortSig.paramTypes.empty());
    EXPECT_TRUE(abortSig.isNoReturn);
}

// Verifies symbol resolution and flavor handling in TargetLibcallRegistry.
TEST_F(EzTripleTestSuite, TestTargetLibcallRegistryDefaultsAndFlavors)
{
    TargetLibcallRegistry reg;

    // GNU defaults
    reg.initDefaults("x86_64", "linux", CrtFlavor::Gnu);
    EXPECT_EQ(reg.getCrtFlavor(), CrtFlavor::Gnu);
    EXPECT_EQ(reg.getLibcallName(LibcallKind::DivI128), "__divti3");
    EXPECT_EQ(reg.getLibcallName(LibcallKind::UDivI128), "__udivti3");
    EXPECT_EQ(reg.getLibcallName(LibcallKind::DivI64), "__divdi3");
    EXPECT_EQ(reg.getLibcallName(LibcallKind::Memcpy), "memcpy");
    EXPECT_EQ(reg.getLibcallName(LibcallKind::SinF64), "sin");

    // MSVC 32-bit x86 defaults
    TargetLibcallRegistry msvcReg;
    msvcReg.initDefaults("i686", "windows", CrtFlavor::Msvc);
    EXPECT_EQ(msvcReg.getCrtFlavor(), CrtFlavor::Msvc);
    EXPECT_EQ(msvcReg.getLibcallName(LibcallKind::DivI64), "_alldiv");
    EXPECT_EQ(msvcReg.getLibcallName(LibcallKind::UDivI64), "_aulldiv");
    EXPECT_EQ(msvcReg.getLibcallName(LibcallKind::Memcpy), "memcpy");
    EXPECT_EQ(msvcReg.getLibcallName(LibcallKind::StackProbe), "__chkstk");
}

// Verifies target-specific symbol overrides and reverse lookup.
TEST_F(EzTripleTestSuite, TestTargetLibcallRegistryCustomOverrides)
{
    TargetLibcallRegistry reg;
    reg.initDefaults("x86_64", "linux", CrtFlavor::Gnu);

    // Custom override
    reg.setLibcallName(LibcallKind::DivI128, "__my_divti3");
    EXPECT_EQ(reg.getLibcallName(LibcallKind::DivI128), "__my_divti3");

    // Reverse lookup finds overridden name
    auto found = reg.findKindByName("__my_divti3");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, LibcallKind::DivI128);

    // Reverse lookup for standard C-rt function
    auto memFound = reg.findKindByName("memcpy");
    ASSERT_TRUE(memFound.has_value());
    EXPECT_EQ(*memFound, LibcallKind::Memcpy);
}

// Verifies hosted vs freestanding behavior in TargetLibcallRegistry.
TEST_F(EzTripleTestSuite, TestTargetLibcallRegistryHostedVsFreestanding)
{
    TargetLibcallRegistry reg;
    reg.initDefaults("x86_64", "linux", CrtFlavor::Gnu);

    // Initially hosted
    EXPECT_TRUE(reg.isHosted());
    EXPECT_TRUE(reg.isAvailable(LibcallKind::SinF64));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::PowF32));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::Memcpy));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::DivI128));

    // Switch to freestanding
    reg.setHosted(false);
    EXPECT_FALSE(reg.isHosted());

    // Hosted math is disabled
    EXPECT_FALSE(reg.isAvailable(LibcallKind::SinF64));
    EXPECT_FALSE(reg.isAvailable(LibcallKind::PowF32));
    EXPECT_FALSE(reg.isAvailable(LibcallKind::Abort));

    // Freestanding memory builtins and compiler-rt remain available
    EXPECT_TRUE(reg.isAvailable(LibcallKind::Memcpy));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::Memset));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::DivI128));
    EXPECT_TRUE(reg.isAvailable(LibcallKind::MulI128));
}

// Verifies registering custom runtime functions with arbitrary signatures.
TEST_F(EzTripleTestSuite, TestTargetLibcallRegistryCustomLibcalls)
{
    TargetLibcallRegistry reg;
    LibcallSignature customSig{
        .returnType = LibcallTypeKind::I64,
        .paramTypes = { LibcallTypeKind::Ptr, LibcallTypeKind::I32 }
    };

    uint16_t id1 = reg.registerCustomLibcall("__custom_hash", customSig);
    uint16_t id2 = reg.registerCustomLibcall("__custom_hash", customSig);
    EXPECT_EQ(id1, id2); // Interning returns identical id

    uint16_t id3 = reg.registerCustomLibcall("__another_helper");
    EXPECT_NE(id1, id3);

    EXPECT_EQ(reg.getCustomLibcallName(id1), "__custom_hash");
    EXPECT_EQ(reg.getCustomLibcallName(id3), "__another_helper");
}

// Verifies opcode and type based inference of LibcallKind.
TEST_F(EzTripleTestSuite, TestTargetLibcallRegistryOpcodeInference)
{
    TargetLibcallRegistry reg;
    auto *tt = getBuilderCtx()->getTypeTable();

    // Integer SDIV
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::SDIV, tt->i32()), LibcallKind::DivI32);
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::SDIV, tt->i64()), LibcallKind::DivI64);
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::SDIV, tt->i128()), LibcallKind::DivI128);

    // Integer UDIV
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::UDIV, tt->i128()), LibcallKind::UDivI128);

    // Integer MUL
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::MUL, tt->i128()), LibcallKind::MulI128);

    // Shifts
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::SHL, tt->i128()), LibcallKind::ShlI128);
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::SAR, tt->i64()), LibcallKind::AShrI64);

    // Float ops
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::FADD, tt->f32()), LibcallKind::AddF32);
    EXPECT_EQ(reg.findKindForOpcode(MirInstructionOpCode::FDIV, tt->f64()), LibcallKind::DivF64);
}
