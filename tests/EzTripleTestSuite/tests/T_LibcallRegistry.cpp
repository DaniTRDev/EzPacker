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

// Verifies exception handling runtime libcalls and signatures
TEST_F(EzTripleTestSuite, TestExceptionLibcalls)
{
    EXPECT_TRUE(isCRuntime(LibcallKind::EzTryEnter));
    EXPECT_TRUE(isCRuntime(LibcallKind::EzTryLeave));
    EXPECT_TRUE(isCRuntime(LibcallKind::EzThrow));
    EXPECT_TRUE(isCRuntime(LibcallKind::EzCatchMatch));
    EXPECT_TRUE(isCRuntime(LibcallKind::EzGetCurrentEx));

    EXPECT_EQ(getLibcallKindName(LibcallKind::EzTryEnter), "EzTryEnter");
    EXPECT_EQ(getLibcallKindName(LibcallKind::EzThrow), "EzThrow");

    EXPECT_EQ(getDefaultLibcallName(LibcallKind::EzTryEnter), "__ez_try_enter");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::EzTryLeave), "__ez_try_leave");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::EzThrow), "__ez_throw");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::EzCatchMatch), "__ez_catch_matches");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::EzGetCurrentEx), "__ez_get_current_exception");

    const auto &throwSig = getCanonicalSignature(LibcallKind::EzThrow);
    EXPECT_EQ(throwSig.returnType, LibcallTypeKind::Void);
    EXPECT_TRUE(throwSig.isNoReturn);
    ASSERT_EQ(throwSig.paramTypes.size(), 3u);
    EXPECT_EQ(throwSig.paramTypes[0], LibcallTypeKind::Ptr);
    EXPECT_EQ(throwSig.paramTypes[1], LibcallTypeKind::Ptr);
    EXPECT_EQ(throwSig.paramTypes[2], LibcallTypeKind::Ptr);

    const auto &enterSig = getCanonicalSignature(LibcallKind::EzTryEnter);
    EXPECT_EQ(enterSig.returnType, LibcallTypeKind::Void);
    ASSERT_EQ(enterSig.paramTypes.size(), 1u);
    EXPECT_EQ(enterSig.paramTypes[0], LibcallTypeKind::Ptr);
}

// Verifies arithmetic and bitwise compiler-rt libcalls: providers, kind names, symbols, and signatures.
TEST_F(EzTripleTestSuite, TestCompilerRtArithmeticAndBitwiseLibcalls)
{
    // Provider checks
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::UDivI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::RemI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::URemI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::MulI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ShlI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::LShrI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::AShrI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::PopcountI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::PopcountI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::PopcountI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ClzI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ClzI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ClzI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CtzI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CtzI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CtzI128));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ParityI32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ParityI64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::ParityI128));

    // Kind names
    EXPECT_EQ(getLibcallKindName(LibcallKind::DivI128), "DivI128");
    EXPECT_EQ(getLibcallKindName(LibcallKind::UDivI128), "UDivI128");
    EXPECT_EQ(getLibcallKindName(LibcallKind::MulI128), "MulI128");
    EXPECT_EQ(getLibcallKindName(LibcallKind::ShlI128), "ShlI128");
    EXPECT_EQ(getLibcallKindName(LibcallKind::PopcountI64), "PopcountI64");
    EXPECT_EQ(getLibcallKindName(LibcallKind::ClzI64), "ClzI64");
    EXPECT_EQ(getLibcallKindName(LibcallKind::CtzI64), "CtzI64");
    EXPECT_EQ(getLibcallKindName(LibcallKind::ParityI64), "ParityI64");

    // Default GNU symbols
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::DivI128), "__divti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::UDivI128), "__udivti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::RemI128), "__modti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::URemI128), "__umodti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::MulI128), "__multi3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ShlI128), "__ashlti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::LShrI128), "__lshrti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::AShrI128), "__ashrti3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::PopcountI32), "__popcountsi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::PopcountI64), "__popcountdi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::PopcountI128), "__popcountti2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ClzI32), "__clzsi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ClzI64), "__clzdi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ClzI128), "__clzti2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CtzI32), "__ctzsi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CtzI64), "__ctzdi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CtzI128), "__ctzti2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ParityI32), "__paritysi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ParityI64), "__paritydi2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::ParityI128), "__parityti2");

    // Canonical signatures
    const auto &popSig = getCanonicalSignature(LibcallKind::PopcountI64);
    EXPECT_EQ(popSig.returnType, LibcallTypeKind::I32);
    ASSERT_EQ(popSig.paramTypes.size(), 1u);
    EXPECT_EQ(popSig.paramTypes[0], LibcallTypeKind::I64);

    const auto &mul128Sig = getCanonicalSignature(LibcallKind::MulI128);
    EXPECT_EQ(mul128Sig.returnType, LibcallTypeKind::I128);
    ASSERT_EQ(mul128Sig.paramTypes.size(), 2u);
    EXPECT_EQ(mul128Sig.paramTypes[0], LibcallTypeKind::I128);
    EXPECT_EQ(mul128Sig.paramTypes[1], LibcallTypeKind::I128);
}

// Verifies soft-float arithmetic, comparison, and conversion compiler-rt libcalls.
TEST_F(EzTripleTestSuite, TestCompilerRtSoftFloatLibcalls)
{
    // Provider checks
    EXPECT_TRUE(isCompilerRt(LibcallKind::AddF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::SubF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::MulF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::AddF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::SubF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::MulF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::DivF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpEqF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpLtF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpLeF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpGtF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpGeF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpNeF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::CmpUnordF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::Int32ToF32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::Int64ToF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::F32ToInt32));
    EXPECT_TRUE(isCompilerRt(LibcallKind::F64ToInt64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::F32ToF64));
    EXPECT_TRUE(isCompilerRt(LibcallKind::F64ToF32));

    // Default GNU symbols
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::AddF32), "__addsf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::SubF32), "__subsf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::MulF32), "__mulsf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::DivF32), "__divsf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::AddF64), "__adddf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::SubF64), "__subdf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::MulF64), "__muldf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::DivF64), "__divdf3");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpEqF32), "__eqsf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpLtF32), "__ltsf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpLeF32), "__lesf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpGtF32), "__gtsf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpGeF32), "__gesf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpNeF32), "__nesf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::CmpUnordF32), "__unordsf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::Int32ToF32), "__floatsisf");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::Int64ToF64), "__floatdidf");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::F32ToInt32), "__fixsfsi");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::F64ToInt64), "__fixdfdi");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::F32ToF64), "__extendsfdf2");
    EXPECT_EQ(getDefaultLibcallName(LibcallKind::F64ToF32), "__truncdfsf2");

    // Signatures
    const auto &addF32Sig = getCanonicalSignature(LibcallKind::AddF32);
    EXPECT_EQ(addF32Sig.returnType, LibcallTypeKind::F32);
    ASSERT_EQ(addF32Sig.paramTypes.size(), 2u);
    EXPECT_EQ(addF32Sig.paramTypes[0], LibcallTypeKind::F32);
    EXPECT_EQ(addF32Sig.paramTypes[1], LibcallTypeKind::F32);

    const auto &cmpEqSig = getCanonicalSignature(LibcallKind::CmpEqF32);
    EXPECT_EQ(cmpEqSig.returnType, LibcallTypeKind::I32);
    ASSERT_EQ(cmpEqSig.paramTypes.size(), 2u);
    EXPECT_EQ(cmpEqSig.paramTypes[0], LibcallTypeKind::F32);
    EXPECT_EQ(cmpEqSig.paramTypes[1], LibcallTypeKind::F32);
}

