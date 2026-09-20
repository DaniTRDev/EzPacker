#include <gtest/gtest.h>
#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture verifying that TestMirFunctionRegisterInfo maintains coherent
 * def-use chains across all MirInstructionBuilder mutations.
 */
class TestMirFunctionRegisterInfo : public MirTestSuiteAsGtest
{
  public:
    MirFunctionRegisterInfo *getRegInfo() { return getTestFunc()->getRegisterInfo(); }
};

/**
 * Verifies initial definition and use registration upon instruction creation.
 */
TEST_F(TestMirFunctionRegisterInfo, TrackOnBuild)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    auto *regInfo = getRegInfo();
    ASSERT_NE(regInfo, nullptr);

    MirRegister *vregDst = oBuilder.buildVReg(getTypeTable()->i32(), "dst");
    MirRegister *vregSrc = oBuilder.buildVReg(getTypeTable()->i32(), "src");

    // ADD vregDst (OUT), vregSrc (IN)
    MirInstruction *addInst = iBuilder.ADD(vregDst, vregSrc);
    ASSERT_NE(addInst, nullptr);

    // Verify destination definition
    EXPECT_EQ(regInfo->getDef(vregDst->getRegId()), addInst);
    EXPECT_EQ(regInfo->getUseCount(vregDst->getRegId()), 0u);

    // Verify source use
    EXPECT_EQ(regInfo->getDef(vregSrc->getRegId()), nullptr);
    EXPECT_EQ(regInfo->getUseCount(vregSrc->getRegId()), 1u);
    EXPECT_TRUE(regInfo->hasOneUse(vregSrc->getRegId()));

    auto usesOpt = regInfo->getUses(vregSrc->getRegId());
    ASSERT_NE(usesOpt, nullptr);
    const auto &uses = *usesOpt;
    ASSERT_EQ(uses.size(), 1u);
    EXPECT_EQ(uses[0].m_userInst, addInst);
    EXPECT_EQ(uses[0].m_operandIndex, 1u);
}

/**
 * Verifies multiple uses of the same virtual register across multiple instructions.
 */
TEST_F(TestMirFunctionRegisterInfo, TrackMultipleUses)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    auto *regInfo = getRegInfo();
    ASSERT_NE(regInfo, nullptr);

    MirRegister *src = oBuilder.buildVReg(getTypeTable()->i32(), "src");
    MirRegister *dst1 = oBuilder.buildVReg(getTypeTable()->i32(), "dst1");
    MirRegister *dst2 = oBuilder.buildVReg(getTypeTable()->i32(), "dst2");

    MirInstruction *inst1 = iBuilder.ADD(dst1, src);
    MirInstruction *inst2 = iBuilder.SUB(dst2, src);

    EXPECT_FALSE(regInfo->hasOneUse(src->getRegId()));
    EXPECT_EQ(regInfo->getUseCount(src->getRegId()), 2u);

    auto usesOpt = regInfo->getUses(src->getRegId());
    ASSERT_NE(usesOpt, nullptr);
    const auto &uses = *usesOpt;
    ASSERT_EQ(uses.size(), 2u);
    EXPECT_EQ(uses[0].m_userInst, inst1);
    EXPECT_EQ(uses[1].m_userInst, inst2);
}

/**
 * Verifies that replacing an operand with swapOperand updates the tracker properly.
 */
TEST_F(TestMirFunctionRegisterInfo, TrackOperandSwap)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    auto *regInfo = getRegInfo();
    ASSERT_NE(regInfo, nullptr);

    MirRegister *dst = oBuilder.buildVReg(getTypeTable()->i32(), "dst");
    MirRegister *srcOld = oBuilder.buildVReg(getTypeTable()->i32(), "srcOld");
    MirRegister *srcNew = oBuilder.buildVReg(getTypeTable()->i32(), "srcNew");

    MirInstruction *inst = iBuilder.ADD(dst, srcOld);
    EXPECT_EQ(regInfo->getUseCount(srcOld->getRegId()), 1u);
    EXPECT_EQ(regInfo->getUseCount(srcNew->getRegId()), 0u);

    // Swap operand at index 1 (srcOld -> srcNew)
    iBuilder.swapOperand(inst, srcNew, 1);

    EXPECT_EQ(regInfo->getUseCount(srcOld->getRegId()), 0u);
    EXPECT_EQ(regInfo->getUseCount(srcNew->getRegId()), 1u);
    EXPECT_TRUE(regInfo->hasOneUse(srcNew->getRegId()));

    auto usesOpt = regInfo->getUses(srcNew->getRegId());
    ASSERT_NE(usesOpt, nullptr);
    const auto &uses = *usesOpt;
    ASSERT_EQ(uses.size(), 1u);
    EXPECT_EQ(uses[0].m_userInst, inst);
    EXPECT_EQ(uses[0].m_operandIndex, 1u);
}

/**
 * Verifies that erasing an instruction clears its definitions and removes its operand uses.
 */
TEST_F(TestMirFunctionRegisterInfo, TrackInstructionErase)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    auto *regInfo = getRegInfo();
    ASSERT_NE(regInfo, nullptr);

    MirRegister *dst = oBuilder.buildVReg(getTypeTable()->i32(), "dst");
    MirRegister *src = oBuilder.buildVReg(getTypeTable()->i32(), "src");

    MirInstruction *inst = iBuilder.ADD(dst, src);
    ASSERT_EQ(regInfo->getDef(dst->getRegId()), inst);
    ASSERT_EQ(regInfo->getUseCount(src->getRegId()), 1u);

    // Erase the instruction
    iBuilder.erase(inst);

    EXPECT_EQ(regInfo->getDef(dst->getRegId()), nullptr);
    EXPECT_EQ(regInfo->getUseCount(src->getRegId()), 0u);
    EXPECT_FALSE(regInfo->hasOneUse(src->getRegId()));
}

/**
 * Verifies tracking when virtual registers are nested inside MirMemory base operands.
 */
TEST_F(TestMirFunctionRegisterInfo, TrackMemoryOperandBaseVReg)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    auto *regInfo = getRegInfo();
    ASSERT_NE(regInfo, nullptr);

    MirRegister *baseReg = oBuilder.buildVReg(getTypeTable()->getPtr(getTypeTable()->i32()), "base");
    MirRegister *valReg = oBuilder.buildVReg(getTypeTable()->i32(), "val");
    MirMemory *memOp = oBuilder.buildMem(getTypeTable()->i32(), baseReg, FlexInt(8));

    // STORE valReg (IN), memOp[baseReg + 8] (IN)
    MirInstruction *storeInst = iBuilder.STORE(valReg, memOp);
    ASSERT_NE(storeInst, nullptr);

    // Verify base virtual register inside MirMemory is tracked as a use
    EXPECT_EQ(regInfo->getUseCount(baseReg->getRegId()), 1u);
    EXPECT_TRUE(regInfo->hasOneUse(baseReg->getRegId()));

    auto usesOpt = regInfo->getUses(baseReg->getRegId());
    ASSERT_NE(usesOpt, nullptr);
    const auto &uses = *usesOpt;
    ASSERT_EQ(uses.size(), 1u);
    EXPECT_EQ(uses[0].m_userInst, storeInst);

    // Clean up instruction and ensure memory base use is dropped
    iBuilder.erase(storeInst);
    EXPECT_EQ(regInfo->getUseCount(baseReg->getRegId()), 0u);
}