#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/Passes/MirPeepholePass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture for the generic middle-end MIR peephole optimization pass.
 */
class MirPeepholePassTest : public MirTestSuiteAsGtest
{
  protected:
    MirBlock *createBlock(const char *name)
    {
        MirFunction *func = getTestFunc();
        MirBlockBuilder builder(getBuilderCtx(), func);
        return builder.build(nullptr, name);
    }
};

TEST_F(MirPeepholePassTest, TestAddZero)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst = ob.buildVReg(i32Type, "dst");
    auto *src = ob.buildVReg(i32Type, "src");
    auto *zero = ob.buildInt(i32Type, FlexInt(0, 32));

    // ADD %dst, %src, 0 -> should become MOV %dst, %src
    ib.ADD(dst, src, zero);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_GT(pass->getResult().m_algebraicSimplifications, 0);

    MirInstruction *firstInst = entry->front();
    ASSERT_NE(firstInst, nullptr);
    EXPECT_EQ(firstInst->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(firstInst->getOperandCount(), 2);
    EXPECT_EQ(firstInst->getOperand(0), dst);
    EXPECT_EQ(firstInst->getOperand(1), src);
}

TEST_F(MirPeepholePassTest, TestAddZeroCommutative)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst = ob.buildVReg(i32Type, "dst");
    auto *src = ob.buildVReg(i32Type, "src");
    auto *zero = ob.buildInt(i32Type, FlexInt(0, 32));

    // ADD %dst, 0, %src -> should become MOV %dst, %src
    ib.ADD(dst, zero, src);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_GT(pass->getResult().m_algebraicSimplifications, 0);

    MirInstruction *firstInst = entry->front();
    ASSERT_NE(firstInst, nullptr);
    EXPECT_EQ(firstInst->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(firstInst->getOperand(0), dst);
    EXPECT_EQ(firstInst->getOperand(1), src);
}

TEST_F(MirPeepholePassTest, TestSubZeroAndSelf)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i64Type = typeTable->i64();
    auto *dst1 = ob.buildVReg(i64Type, "dst1");
    auto *dst2 = ob.buildVReg(i64Type, "dst2");
    auto *src = ob.buildVReg(i64Type, "src");
    auto *zero = ob.buildInt(i64Type, FlexInt(0, 64));

    // SUB %dst1, %src, 0 -> MOV %dst1, %src
    ib.SUB(dst1, src, zero);

    // SUB %dst2, %src, %src -> MOV %dst2, 0
    ib.SUB(dst2, src, src);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_algebraicSimplifications, 2);

    auto it = entry->getInstructions().begin();
    MirInstruction *inst1 = *it++;
    MirInstruction *inst2 = *it;

    EXPECT_EQ(inst1->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(inst1->getOperand(0), dst1);
    EXPECT_EQ(inst1->getOperand(1), src);

    EXPECT_EQ(inst2->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(inst2->getOperand(0), dst2);
    ASSERT_TRUE(inst2->getOperand(1)->isOfType<MirInteger>());
    EXPECT_TRUE(inst2->getOperand(1)->get<MirInteger>()->getValue().isZero());
}

TEST_F(MirPeepholePassTest, TestMulOneAndZero)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst1 = ob.buildVReg(i32Type, "dst1");
    auto *dst2 = ob.buildVReg(i32Type, "dst2");
    auto *src = ob.buildVReg(i32Type, "src");
    auto *one = ob.buildInt(i32Type, FlexInt(1, 32));
    auto *zero = ob.buildInt(i32Type, FlexInt(0, 32));

    // IMUL %dst1, %src, 1 -> MOV %dst1, %src
    ib.IMUL(dst1, src, one);
    // IMUL %dst2, %src, 0 -> MOV %dst2, 0
    ib.IMUL(dst2, src, zero);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_algebraicSimplifications, 2);

    auto it = entry->getInstructions().begin();
    MirInstruction *inst1 = *it++;
    MirInstruction *inst2 = *it;

    EXPECT_EQ(inst1->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(inst1->getOperand(1), src);

    EXPECT_EQ(inst2->getOpCode(), MirInstructionOpCode::MOV);
    ASSERT_TRUE(inst2->getOperand(1)->isOfType<MirInteger>());
    EXPECT_TRUE(inst2->getOperand(1)->get<MirInteger>()->getValue().isZero());
}

TEST_F(MirPeepholePassTest, TestBitwiseIdentities)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *src = ob.buildVReg(i32Type, "src");
    auto *zero = ob.buildInt(i32Type, FlexInt(0, 32));
    auto *allOnes = ob.buildInt(i32Type, ~FlexInt(0, 32));

    auto *dAndZero = ob.buildVReg(i32Type, "dAndZero");
    auto *dAndOnes = ob.buildVReg(i32Type, "dAndOnes");
    auto *dAndSelf = ob.buildVReg(i32Type, "dAndSelf");
    auto *dOrZero = ob.buildVReg(i32Type, "dOrZero");
    auto *dOrSelf = ob.buildVReg(i32Type, "dOrSelf");
    auto *dXorZero = ob.buildVReg(i32Type, "dXorZero");
    auto *dXorSelf = ob.buildVReg(i32Type, "dXorSelf");

    ib.AND(dAndZero, src, zero);       // -> MOV 0
    ib.AND(dAndOnes, src, allOnes);    // -> MOV src
    ib.AND(dAndSelf, src, src);        // -> MOV src
    ib.OR(dOrZero, src, zero);         // -> MOV src
    ib.OR(dOrSelf, src, src);          // -> MOV src
    ib.XOR(dXorZero, src, zero);       // -> MOV src
    ib.XOR(dXorSelf, src, src);        // -> MOV 0

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_algebraicSimplifications, 7);

    for (MirInstruction *inst : entry->getInstructions())
    {
        EXPECT_EQ(inst->getOpCode(), MirInstructionOpCode::MOV);
    }
}

TEST_F(MirPeepholePassTest, TestShiftZero)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst = ob.buildVReg(i32Type, "dst");
    auto *src = ob.buildVReg(i32Type, "src");
    auto *zero = ob.buildInt(i32Type, FlexInt(0, 32));

    ib.SHL(dst, src, zero);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_algebraicSimplifications, 1);

    MirInstruction *inst = entry->front();
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(inst->getOperand(0), dst);
    EXPECT_EQ(inst->getOperand(1), src);
}

TEST_F(MirPeepholePassTest, TestRedundantMoves)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *rA = ob.buildVReg(i32Type, "rA");
    auto *rB = ob.buildVReg(i32Type, "rB");

    // 1. Identity self-move: MOV %rA, %rA -> should be erased
    ib.MOV(rA, rA);

    // 2. Reciprocal moves: MOV %rA, %rB; MOV %rB, %rA -> second should be erased
    ib.MOV(rA, rB);
    ib.MOV(rB, rA);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_redundantMovesEliminated, 2);

    // Only MOV %rA, %rB should remain
    EXPECT_EQ(entry->getInstrCount(), 1);
    MirInstruction *remaining = entry->front();
    ASSERT_NE(remaining, nullptr);
    EXPECT_EQ(remaining->getOperand(0), rA);
    EXPECT_EQ(remaining->getOperand(1), rB);
}

TEST_F(MirPeepholePassTest, TestDeadInstructionsAfterTerminator)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *retVal = ob.buildVReg(i32Type, "retVal");
    auto *deadReg = ob.buildVReg(i32Type, "deadReg");
    auto *imm42 = ob.buildInt(i32Type, FlexInt(42, 32));

    ib.RET(retVal);
    ib.MOV(deadReg, imm42); // Dead!
    ib.MOV(deadReg, imm42); // Dead!

    EXPECT_EQ(entry->getInstrCount(), 3);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_deadInstructionsEliminated, 2);
    EXPECT_EQ(entry->getInstrCount(), 1);
    EXPECT_EQ(entry->front()->getOpCode(), MirInstructionOpCode::RET);
}

TEST_F(MirPeepholePassTest, TestJumpToNextBlock)
{
    auto *ctx = getBuilderCtx();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirBlock *nextBlock = createBlock("nextBlock");
    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // Entry jumps to nextBlock which is immediately next in layout!
    auto *blockRef = ob.buildRef(nextBlock);
    ib.JMP(blockRef);

    EXPECT_EQ(entry->getInstrCount(), 1);

    auto *pass = runPass<MirPeepholePass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_EQ(pass->getResult().m_branchesSimplified, 1);
    EXPECT_EQ(entry->getInstrCount(), 0);
}
