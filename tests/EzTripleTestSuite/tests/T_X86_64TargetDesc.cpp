#include "EzTripleTestSuite.h"
#include "Targets/X86_64/X86_64TargetDesc.h"
#include "Targets/X86_64/X86_64ElfBinaryDesc.h"
#include "Targets/X86_64/X86_64CoffBinaryDesc.h"
#include "Descriptors/TargetRelocationResolver.h"
#include "GenericCodeEmitter.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Function/CallingConvDesc.h"
#include "CodeSection.h"
#include "Legalizer/LegalityQuery.h"
#include "x86_64TargetInstructionTable.h"

using namespace EzTriple;

// Verifies basic descriptor metadata, displacement type, RIP register id, and GPR class.
TEST_F(EzTripleTestSuite, TestX86_64TargetDescInitialization)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_STREQ(target.getName(), "x86_64");
    EXPECT_EQ(target.getStackSlotSize(), 8u);

    // Displacement type is i64
    EXPECT_EQ(target.getMemOperandDisplacementType(), getBuilderCtx()->getTypeTable()->i64());

    // RIP instruction pointer register
    MirRegisterRef rip = target.getInstructionPtrReg();
    EXPECT_EQ(rip.getId(), 16u);

    // GPR class
    MirRegisterClass *gpr = target.getGprClass();
    ASSERT_NE(gpr, nullptr);
    EXPECT_STREQ(gpr->getName(), "GPR64");
}

// Verifies the GPR and FPR banks expose their classes, register counts, and aliases.
TEST_F(EzTripleTestSuite, TestX86_64RegisterBanksAndClasses)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto banks = target.getAvailableRegisterBanks();
    ASSERT_EQ(banks.size(), 2u);

    // GPR Bank
    MirRegisterBank *gprBank = banks[0];
    ASSERT_NE(gprBank, nullptr);
    EXPECT_STREQ(gprBank->getName(), "GPR");

    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");
    MirRegisterClass *gpr32 = gprBank->getClass("GPR32");
    MirRegisterClass *gpr16 = gprBank->getClass("GPR16");
    MirRegisterClass *gpr8 = gprBank->getClass("GPR8");

    ASSERT_NE(gpr64, nullptr);
    ASSERT_NE(gpr32, nullptr);
    ASSERT_NE(gpr16, nullptr);
    ASSERT_NE(gpr8, nullptr);

    // Check register count in GPR bank (16 registers)
    EXPECT_EQ(gpr64->getRegs().size(), 16u);
    EXPECT_EQ(gpr32->getRegs().size(), 16u);
    EXPECT_EQ(gpr16->getRegs().size(), 16u);
    EXPECT_EQ(gpr8->getRegs().size(), 16u);

    // Check register lookups
    EXPECT_NE(gpr64->getReg("rax"), nullptr);
    EXPECT_NE(gpr32->getReg("eax"), nullptr);
    EXPECT_NE(gpr16->getReg("ax"), nullptr);
    EXPECT_NE(gpr8->getReg("al"), nullptr);

    EXPECT_NE(gpr64->getReg("rsp"), nullptr);
    EXPECT_NE(gpr64->getReg("rbp"), nullptr);

    // FPR Bank
    MirRegisterBank *fprBank = banks[1];
    ASSERT_NE(fprBank, nullptr);
    EXPECT_STREQ(fprBank->getName(), "FPR");

    MirRegisterClass *fpr64 = fprBank->getClass("FPR64");
    MirRegisterClass *fpr32 = fprBank->getClass("FPR32");
    ASSERT_NE(fpr64, nullptr);
    ASSERT_NE(fpr32, nullptr);

    EXPECT_EQ(fpr64->getRegs().size(), 16u);
    EXPECT_EQ(fpr32->getRegs().size(), 16u);
    EXPECT_NE(fpr64->getReg("xmm0"), nullptr);
    EXPECT_NE(fpr64->getReg("xmm15"), nullptr);
}

// Verifies the SysV and Win64 conventions with alignment, shadow space, and stack direction.
TEST_F(EzTripleTestSuite, TestX86_64CallingConventions)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto convs = target.getAvailableCallingConventions();
    ASSERT_EQ(convs.size(), 2u);

    CallingConvDesc *sysV = target.getSysVCallingConv();
    ASSERT_NE(sysV, nullptr);
    EXPECT_STREQ(sysV->getName(), "SysV_AMD64");
    EXPECT_EQ(sysV->getStackAlignment(), 16u);
    EXPECT_EQ(sysV->getShadowSpaceSize(), 0u);
    EXPECT_TRUE(sysV->doesStackGrowsDownwards());

    CallingConvDesc *win64 = target.getWin64CallingConv();
    ASSERT_NE(win64, nullptr);
    EXPECT_STREQ(win64->getName(), "Win64");
    EXPECT_EQ(win64->getStackAlignment(), 16u);
    EXPECT_EQ(win64->getShadowSpaceSize(), 32u);
    EXPECT_TRUE(win64->doesStackGrowsDownwards());
}

// Verifies all target subsystems, target instruction descriptors, and libcall mappings.
TEST_F(EzTripleTestSuite, TestX86_64Subsystems)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_NE(target.getLegalizer(), nullptr);
    EXPECT_NE(target.getLegalizerInfo(), nullptr);
    EXPECT_NE(target.getInstructionSelector(), nullptr);
    EXPECT_NE(target.getAddressingModeMatcher(), nullptr);
    EXPECT_NE(target.getRegisterAllocator(), nullptr);
    EXPECT_NE(target.getFrameLowerer(), nullptr);

    // Target instruction retrieval
    auto *descADD64rr = x86_64TargetInst::getTargetDesc(x86_64TargetInst::ADD64rr);
    ASSERT_NE(descADD64rr, nullptr);
    EXPECT_STREQ(descADD64rr->getName(), "ADD64rr");

    auto *descMOV64rr = x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV64rr);
    ASSERT_NE(descMOV64rr, nullptr);
    EXPECT_STREQ(descMOV64rr->getName(), "MOV64rr");

    // Libcall resolution
    EXPECT_EQ(target.getLibcallStr(1), "__divdi3");
    EXPECT_EQ(target.getLibcallStr(2), "__udivdi3");
    EXPECT_EQ(target.getLibcallStr(3), "__moddi3");
    EXPECT_EQ(target.getLibcallStr(4), "__umoddi3");
    EXPECT_EQ(target.getLibcallStr(5), "__muldi3");
}

// Verifies the ELF and COFF binary descriptors and their standard sections.
TEST_F(EzTripleTestSuite, TestX86_64BinaryDescriptors)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto binaries = target.getAvailableBinaryDescriptors();
    ASSERT_EQ(binaries.size(), 2u);

    TargetBinaryDesc *elf = target.getElfBinaryDesc();
    ASSERT_NE(elf, nullptr);
    EXPECT_STREQ(elf->getName(), "x86_64-elf");
    EXPECT_TRUE(elf->isLittleEndian());
    EXPECT_NE(elf->getSection(SectionType::Text), nullptr);
    EXPECT_NE(elf->getSection(SectionType::Data), nullptr);
    EXPECT_NE(elf->getSection(SectionType::ReadOnly), nullptr);
    EXPECT_NE(elf->getSection(SectionType::NonInitialized), nullptr);

    TargetBinaryDesc *coff = target.getCoffBinaryDesc();
    ASSERT_NE(coff, nullptr);
    EXPECT_STREQ(coff->getName(), "x86_64-coff");
    EXPECT_TRUE(coff->isLittleEndian());
    EXPECT_NE(coff->getSection(SectionType::Text), nullptr);
    EXPECT_NE(coff->getSection(SectionType::Data), nullptr);
    EXPECT_NE(coff->getSection(SectionType::ReadOnly), nullptr);
    EXPECT_NE(coff->getSection(SectionType::NonInitialized), nullptr);
}

// Verifies the emitter factory, register bank factory, and stable relocation resolver.
TEST_F(EzTripleTestSuite, TestX86_64EmitterBankFactoryAndResolverSurface)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    // createCodeEmitter returns a fresh target emitter.
    GenericCodeEmitter *emitter = target.createCodeEmitter();
    ASSERT_NE(emitter, nullptr);
    delete emitter;

    // createRegisterBank registers a new bank accessible through the descriptor.
    const size_t initialBankCount = target.getAvailableRegisterBanks().size();
    MirRegisterBank *bank = target.createRegisterBank("TMP");
    ASSERT_NE(bank, nullptr);
    EXPECT_STREQ(bank->getName(), "TMP");
    EXPECT_EQ(target.getAvailableRegisterBanks().size(), initialBankCount + 1);

    // The relocation resolver is stable and non-null.
    TargetRelocationResolver *resolver = target.getRelocationResolver();
    ASSERT_NE(resolver, nullptr);
    EXPECT_EQ(target.getRelocationResolver(), resolver);
}
