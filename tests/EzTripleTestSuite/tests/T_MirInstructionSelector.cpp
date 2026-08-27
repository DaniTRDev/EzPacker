#include "EzTripleTestSuite.h"
#include "Instruction/MirInstruction.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "Operand/MirOperandBuilder.h"

class MirInstructionSelectorTest : public EzTripleTestSuite
{
};

TEST_F(MirInstructionSelectorTest, TestInstructionSelectionDispatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *rd = ob.buildVReg(typeTable->i32(), "rd");
    MirRegister *rs1 = ob.buildVReg(typeTable->i32(), "rs1");
    MirRegister *rs2 = ob.buildVReg(typeTable->i32(), "rs2");

    ib.ADD(rd, rs1, rs2);
    ib.SUB(rd, rs1, rs2);

    EXPECT_EQ(block->getInstructions().size(), 2);

    auto *isel = getTargetDesc()->getInstructionSelector();
    bool ok = isel->selectBlock(ctx, block);
    EXPECT_TRUE(ok);

    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    EXPECT_EQ(mockIsel->m_selectedCount, 2);
}

TEST_F(MirInstructionSelectorTest, TestInstructionSelectorPass)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_pass_test", typeTable->i32());
    auto *block0 = func->getEntryPoint();
    auto *block1 = createBlock(func, "b1");

    MirInstructionBuilder ib0(ctx, block0, InsertionType::Append);
    MirInstructionBuilder ib1(ctx, block1, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *rd = ob.buildVReg(typeTable->i32(), "r");
    ib0.ADD(rd, rd, rd);
    ib1.MUL(rd, rd, rd);

    MirInstructionSelectorPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto res = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_executed);

    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    EXPECT_EQ(mockIsel->m_selectedCount, 2);
}
