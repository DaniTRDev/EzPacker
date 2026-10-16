#include "EzTripleTestSuite.h"
#include "RiscV64TargetDesc.h"
#include "Descriptors/RiscV64ElfBinaryDesc.h"
#include "Descriptors/TargetRelocationResolver.h"
#include "GenericCodeEmitter.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirOperandBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Function/CallingConvDesc.h"
#include "CodeSection.h"
#include "Legalizer/LegalityQuery.h"
#include "riscv64TargetInstructionTable.h"
#include "riscv64LegalizerRules.h"

using namespace EzTargets::RiscV64;

// Verifies basic descriptor metadata, stack slot size, PC register id, and GPR class.
TEST_F(EzTripleTestSuite, TestRiscV64TargetDescInitialization)
{
    RiscV64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_STREQ(target.getName(), "riscv64");
    EXPECT_EQ(target.getStackSlotSize(), 8u);

    // Displacement type is i64
    EXPECT_EQ(target.getMemOperandDisplacementType(), getBuilderCtx()->getTypeTable()->i64());

    // PC instruction pointer register
    MirRegisterRef pc = target.getInstructionPtrReg();
    EXPECT_EQ(pc.getId(), 32u);

    // GPR class
    MirRegisterClass *gpr = target.getGprClass();
    ASSERT_NE(gpr, nullptr);
    EXPECT_STREQ(gpr->getName(), "GPR64");
}

// Verifies the GPR and FPR banks expose their classes, register counts, and lookups.
TEST_F(EzTripleTestSuite, TestRiscV64RegisterBanksAndClasses)
{
    RiscV64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto banks = target.getAvailableRegisterBanks();
    ASSERT_EQ(banks.size(), 2u);

    // GPR Bank
    MirRegisterBank *gprBank = banks[0];
    ASSERT_NE(gprBank, nullptr);
    EXPECT_STREQ(gprBank->getName(), "GPR");

    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");
    MirRegisterClass *gpr32 = gprBank->getClass("GPR32");
    ASSERT_NE(gpr64, nullptr);
    ASSERT_NE(gpr32, nullptr);

    EXPECT_EQ(gpr64->getRegs().size(), 32u);
    EXPECT_EQ(gpr32->getRegs().size(), 32u);

    // Verify key ABI registers by lookup
    EXPECT_NE(gpr64->getReg("zero"), nullptr);
    EXPECT_NE(gpr64->getReg("ra"), nullptr);
    EXPECT_NE(gpr64->getReg("sp"), nullptr);
    EXPECT_NE(gpr64->getReg("s0"), nullptr);
    EXPECT_NE(gpr64->getReg("a0"), nullptr);

    // FPR Bank
    MirRegisterBank *fprBank = banks[1];
    ASSERT_NE(fprBank, nullptr);
    EXPECT_STREQ(fprBank->getName(), "FPR");

    MirRegisterClass *fpr64 = fprBank->getClass("FPR64");
    MirRegisterClass *fpr32 = fprBank->getClass("FPR32");
    ASSERT_NE(fpr64, nullptr);
    ASSERT_NE(fpr32, nullptr);

    EXPECT_EQ(fpr64->getRegs().size(), 32u);
    EXPECT_EQ(fpr32->getRegs().size(), 32u);
    EXPECT_NE(fpr64->getReg("ft0"), nullptr);
    EXPECT_NE(fpr64->getReg("fa0"), nullptr);
}

// Verifies calling convention descriptor properties and callee-saved registers.
TEST_F(EzTripleTestSuite, TestRiscV64CallingConvention)
{
    RiscV64TargetDesc target(getBuilderCtx());
    target.initialize();

    CallingConvDesc *cc = target.getDefaultCallingConv();
    ASSERT_NE(cc, nullptr);
    EXPECT_STREQ(cc->getName(), "LP64D");
    EXPECT_EQ(cc->getStackAlignment(), 16u);
    EXPECT_TRUE(cc->doesStackGrowsDownwards());

    // Callee-saved registers
    auto &calleeSaved = cc->getCalleeSavedRegs(target.getGprClass());
    EXPECT_GE(calleeSaved.size(), 12u);
    bool hasS0 = false;
    for (const auto &reg : calleeSaved)
    {
        if (reg.getId() == 8u)
        {
            hasS0 = true;
            break;
        }
    }
    EXPECT_TRUE(hasS0);
}

// Verifies ELF binary descriptor initialization and object format.
TEST_F(EzTripleTestSuite, TestRiscV64ElfBinaryDescriptor)
{
    RiscV64TargetDesc target(getBuilderCtx());
    target.initialize();

    TargetBinaryDesc *elf = target.getElfBinaryDesc();
    ASSERT_NE(elf, nullptr);
    EXPECT_STREQ(elf->getName(), "riscv64-elf");
    EXPECT_TRUE(elf->isLittleEndian());
    EXPECT_NE(elf->getSection(SectionType::Text), nullptr);
    EXPECT_NE(elf->getSection(SectionType::Data), nullptr);
    EXPECT_NE(elf->getSection(SectionType::ReadOnly), nullptr);
    EXPECT_NE(elf->getSection(SectionType::NonInitialized), nullptr);
}

// Verifies that code emitter, instruction selector, frame lowerer, and register allocator subsystems exist.
TEST_F(EzTripleTestSuite, TestRiscV64TargetSubsystems)
{
    RiscV64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_NE(target.getLegalizer(), nullptr);
    EXPECT_NE(target.getLegalizerInfo(), nullptr);
    EXPECT_NE(target.getInstructionSelector(), nullptr);
    EXPECT_NE(target.getRegisterAllocator(), nullptr);
    EXPECT_NE(target.getFrameLowerer(), nullptr);

    // Target instruction retrieval
    auto *descADD64rr = riscv64TargetInst::getTargetDesc(riscv64TargetInst::ADD64rr);
    ASSERT_NE(descADD64rr, nullptr);
    EXPECT_STREQ(descADD64rr->getName(), "ADD64rr");

    auto *descRET = riscv64TargetInst::getTargetDesc(riscv64TargetInst::RET);
    ASSERT_NE(descRET, nullptr);
    EXPECT_STREQ(descRET->getName(), "RET");
}
