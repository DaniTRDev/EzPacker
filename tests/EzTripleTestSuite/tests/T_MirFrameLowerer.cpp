#include "EzTripleTestSuite.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperandBuilder.h"

class MirFrameLowererTest : public EzTripleTestSuite
{
};

TEST_F(MirFrameLowererTest, TestFrameLayoutCalculation)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_calc", typeTable->i32());

    auto *frame = func->getStackFrame();
    auto *obj1 = frame->createStaticStackObj(typeTable->i32());
    auto *obj2 = frame->createStaticStackObj(typeTable->i64());

    EXPECT_NE(obj1, nullptr);
    EXPECT_NE(obj2, nullptr);

    auto *frameLowerer = getTargetDesc()->getFrameLowerer();
    FrameLowererCtx flCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    frameLowerer->calculateFrameLayout(flCtx);

    // Frame size should be calculated and 16-byte aligned in analysis data
    EXPECT_GT(func->getAnalysisData()->m_totalFrameSize, 0);
    EXPECT_EQ(func->getAnalysisData()->m_totalFrameSize % 16, 0);
    EXPECT_LE(obj1->m_offset, 0);
}

TEST_F(MirFrameLowererTest, TestStackObjectReferenceRewriting)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_rewrite", typeTable->i32());
    auto *block = func->getEntryPoint();

    auto *frame = func->getStackFrame();
    auto *obj = frame->createStaticStackObj(typeTable->i32());

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *vreg = ob.buildVReg(typeTable->i32(), "val");
    MirReference *stackRef = ob.buildRef(obj);

    // LOAD vreg, stackRef
    ib.LOAD(vreg, stackRef);

    auto *frameLowerer = getTargetDesc()->getFrameLowerer();
    FrameLowererCtx flCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    frameLowerer->calculateFrameLayout(flCtx);
    frameLowerer->lowerStackObjectReferences(flCtx);

    // Verify the operand was rewritten to a memory operand referencing FP (RBP)
    MirInstruction *loadInst = block->getInstructions().front();
    MirOperand *addrOp = loadInst->getOperand(1);
    EXPECT_TRUE(addrOp->isOfType<MirMemory>());
}

TEST_F(MirFrameLowererTest, TestFrameLowererPassRun)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_pass", typeTable->i32());
    auto *block = func->getEntryPoint();

    auto *frame = func->getStackFrame();
    frame->createStaticStackObj(typeTable->i64());

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    MirRegister *vreg = ob.buildVReg(typeTable->i32(), "res");
    ib.RET(vreg);

    MirFrameLowererPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
}
