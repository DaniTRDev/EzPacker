#include "gtest/gtest.h"
#include "EzMirTestSuite/EzMirTestSuite.h"

/**
 * This test defines certain special operations to make the creation of tests easier.
 */
class LivenessAnalysisTest : public MirTestSuiteAsGtest
{
  public:
    // Helper to quickly allocate registers in tests
    MirRegister *createInt32Reg(std::string_view name)
    {
        return MirOperandBuilder(getBuilderCtx())
                .build<MirRegister>(getTypeTable()->i32(), true, getBuilderCtx()->createId(), nullptr, name.data());
    }

  private:
};

TEST_F(LivenessAnalysisTest, TestStraightLineCode)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder opBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirInstructionInsertionPoint entryIP{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder builder(ctx, entryIP);

    // Setup 3 variables
    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *v1 = createInt32Reg("v1");
    MirRegister *v2 = createInt32Reg("v2");
    MirInteger *imm10 = opBuilder.buildInt(getTypeTable()->i32(), 10);

    // Sequence:
    // 1. MOV %v0, 10      -> DEF: %v0
    // 2. MOV %v1, %v0     -> USE: %v0, DEF: %v1
    // 3. RET %v1          -> USE: %v1
    builder.MOV(v0, imm10);
    builder.MOV(v1, v0);
    builder.RET(v1);

    // Run Analysis. This pass is added and then run because it is an ANALYSIS pass.
    getPassManager()->addPass<LivenessAnalysis>(ctx);
    getPassManager()->addPass<CodeFlowAnalysis>(ctx);

    LivenessAnalysis *pass = getPassManager()->getAnalysis<LivenessAnalysis>(getFunctions());
    LivenessAnalysisVerifier verifier(pass);

    verifier.executed().succeeded();

    size_t blockId = entryPoint->getId();

    // Local Verification
    verifier.localDef(blockId, v0->getRegId()).localDef(blockId, v1->getRegId());

    // Because %v0 is defined *before* it is used in line 2, it should NOT be in the block-local USE set.
    // The only things in the local USE set are things read *before* a local definition.
    verifier.notLocalUse(blockId, v0->getRegId()).notLocalUse(blockId, v1->getRegId());

    // Global Verification (Empty boundary conditions for basic block terminal functions)
    verifier.liveInCount(blockId, 0).liveOutCount(blockId, 0);
}

TEST_F(LivenessAnalysisTest, TestBranchingLiveness)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder opBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *thenBlock = blockBuilder.build(nullptr, "then");
    MirBlock *elseBlock = blockBuilder.build(nullptr, "else");
    MirBlock *mergeBlock = blockBuilder.build(nullptr, "merge");

    // Enforce layout sequence for CFG fallthrough compatibility
    auto &blocks = getTestFunc()->getBlocks();
    blocks.clear();
    blocks.push_back(entryPoint);
    blocks.push_back(thenBlock);
    blocks.push_back(elseBlock);
    blocks.push_back(mergeBlock);

    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *vCond = createInt32Reg("vCond");
    MirInteger *imm5 = opBuilder.buildInt(getTypeTable()->i32(), 5);

    // Entry Block: Define %v0, define condition, branch
    MirInstructionInsertionPoint entryIP{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder entryBuilder(ctx, entryIP);
    entryBuilder.MOV(v0, imm5);
    entryBuilder.CMP(vCond, imm5);
    entryBuilder.JE(opBuilder.buildRef(thenBlock));
    // Fallthrough to elseBlock automatically

    // Then Block: Reads %v0
    MirInstructionInsertionPoint thenIP{ .m_type = InsertionType::InsertAfter, .m_block = thenBlock };
    MirInstructionBuilder thenBuilder(ctx, thenIP);
    thenBuilder.MOV(createInt32Reg("unused1"), v0);
    thenBuilder.JMP(opBuilder.buildRef(mergeBlock));

    // Else Block: Overwrites or ignores %v0 completely (Does NOT use it)
    MirInstructionInsertionPoint elseIP{ .m_type = InsertionType::InsertAfter, .m_block = elseBlock };
    MirInstructionBuilder elseBuilder(ctx, elseIP);
    elseBuilder.MOV(createInt32Reg("unused2"), imm5);
    elseBuilder.JMP(opBuilder.buildRef(mergeBlock));

    // Merge Block: Clean exit
    MirInstructionInsertionPoint mergeIP{ .m_type = InsertionType::InsertAfter, .m_block = mergeBlock };
    MirInstructionBuilder mergeBuilder(ctx, mergeIP);
    mergeBuilder.RET(imm5);

    // Run Dataflow Pipelines
    getPassManager()->addPass<LivenessAnalysis>(ctx);
    getPassManager()->addPass<CodeFlowAnalysis>(ctx);

    LivenessAnalysis *pass = getPassManager()->getAnalysis<LivenessAnalysis>(getFunctions());
    LivenessAnalysisVerifier verifier(pass);

    verifier.executed().succeeded();

    // Global Verifications:
    // %v0 MUST be live out of entryPoint
    verifier.liveOut(entryPoint->getId(), v0->getRegId());

    // %v0 MUST be live into the thenBlock (since it reads it)
    verifier.liveIn(thenBlock->getId(), v0->getRegId());

    // %v0 should NOT be live into elseBlock (since it doesn't read it, nor do its successors)
    verifier.notLiveIn(elseBlock->getId(), v0->getRegId());
}

TEST_F(LivenessAnalysisTest, TestInPlaceArithmetic)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder opBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirInstructionInsertionPoint entryIP{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder builder(ctx, entryIP);

    MirRegister *v0 = createInt32Reg("v0");
    MirInteger *imm1 = opBuilder.buildInt(getTypeTable()->i32(), 1);

    // Sequence:
    // 1. ADD %v0, 1 -> Because ADD destination is ReadWrite, this reads %v0 BEFORE rewriting it.
    //                  Therefore, %v0 is a local USE, and its value must flow from outside this block.
    builder.ADD(v0, imm1);

    getPassManager()->addPass<LivenessAnalysis>(ctx);
    getPassManager()->addPass<CodeFlowAnalysis>(ctx);

    LivenessAnalysis *pass = getPassManager()->getAnalysis<LivenessAnalysis>(getFunctions());
    LivenessAnalysisVerifier verifier(pass);

    verifier.executed().succeeded();

    size_t blockId = entryPoint->getId();

    // Local Verification
    // %v0 must be both defined AND used locally by this single basic block
    verifier.localDef(blockId, v0->getRegId());
    verifier.localUse(blockId, v0->getRegId());

    // Global Verification:
    // Because it was used before a pure overwrite, it is expected to be a LIVE-IN to this block!
    verifier.liveIn(blockId, v0->getRegId());
}
