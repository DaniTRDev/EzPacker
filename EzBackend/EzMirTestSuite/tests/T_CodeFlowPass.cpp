#include <gtest/gtest.h>
#include "../include/EzMirTestSuite.h"

class TestCodeFlowPass : public MirTestSuiteAsGtest
{
  public:
};

TEST_F(TestCodeFlowPass, TestFuncDoesNotHaveSuccessorsOrPredecessors)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    size_t entryPointId = entryPoint->getId();

    CodeFlowAnalysis *pass = runPass<CodeFlowAnalysis>(ctx);
    CodeFlowAnalysisVerifier verifier(pass, ctx);

    // Verify metadata invariants provided by the base Pass framework first
    verifier.executed().succeeded();

    // Verify dataflow graph connectivity state
    verifier.predecessorCount(entryPointId, 0);
    verifier.successorCount(entryPointId, 0);
    verifier.exitBlock(entryPointId);
}

TEST_F(TestCodeFlowPass, Test1Successor)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirTypeTable *typeTable = getTypeTable();

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *successor = blockBuilder.build(nullptr, "");

    MirOperandBuilder operandBuilder(ctx);

    // Set up the insertion flow to terminate the entry block with a JMP to the successor
    MirInstructionInsertionPoint ip{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder instrBuilder(ctx, ip);

    instrBuilder.JMP(operandBuilder.buildRef(successor));

    CodeFlowAnalysis *pass = runPass<CodeFlowAnalysis>(ctx);
    CodeFlowAnalysisVerifier verifier(pass, ctx);

    // Assert structural evaluation completeness
    verifier.executed().succeeded();

    // Assert graph edge details
    verifier.predecessorCount(entryPoint->getId(), 0);
    verifier.successorCount(entryPoint->getId(), 1);
    verifier.predecessorCount(successor->getId(), 1);
    verifier.successorCount(successor->getId(), 0);

    // Check mapping intersections
    verifier.successor(entryPoint->getId(), successor->getId());
    verifier.predecessor(successor->getId(), entryPoint->getId());

    // Evaluate global path connectivity via DFS engine
    verifier.reachable(entryPoint->getId(), successor->getId());
    verifier.unreachable(successor->getId(), entryPoint->getId()); // CFG is directed, cannot walk backwards
    verifier.exitBlock(successor->getId());
}

TEST_F(TestCodeFlowPass, TestLowLevelConditionalBranch)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirTypeTable *typeTable = getTypeTable();
    MirOperandBuilder operandBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *thenBlock = blockBuilder.build(nullptr, "");
    MirBlock *elseBlock = blockBuilder.build(nullptr, "");
    MirBlock *mergeBlock = blockBuilder.build(nullptr, "");

    MirRegister *op1 = operandBuilder.buildVReg(typeTable->i32(), "reg1");
    MirRegister *op2 = operandBuilder.buildVReg(typeTable->i32(), "reg2");

    // Populate Entry Block: CMP -> JNE -> JMP
    MirInstructionInsertionPoint entryIP{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder entryBuilder(ctx, entryIP);

    // Evaluate comparison (updates virtual status flags)
    entryBuilder.CMP(op1, op2);

    // If Not Equal, jump to the else block
    entryBuilder.JNE(operandBuilder.buildRef(elseBlock));

    // Otherwise, unconditionally jump to the then block
    entryBuilder.JMP(operandBuilder.buildRef(thenBlock));

    // Populate 'Then' and 'Else' Blocks to route to Merge
    MirInstructionInsertionPoint thenIP{ .m_type = InsertionType::InsertAfter, .m_block = thenBlock };
    MirInstructionBuilder thenBuilder(ctx, thenIP);
    thenBuilder.JMP(operandBuilder.buildRef(mergeBlock));

    MirInstructionInsertionPoint elseIP{ .m_type = InsertionType::InsertAfter, .m_block = elseBlock };
    MirInstructionBuilder elseBuilder(ctx, elseIP);
    elseBuilder.JMP(operandBuilder.buildRef(mergeBlock));

    // Run and Verify Analysis
    CodeFlowAnalysis *pass = runPass<CodeFlowAnalysis>(ctx);
    CodeFlowAnalysisVerifier verifier(pass, ctx);

    verifier.executed().succeeded();

    // Verify that the entry point successfully registered 2 distinct edge routes
    verifier.successorCount(entryPoint->getId(), 2);
    verifier.successor(entryPoint->getId(), thenBlock->getId());
    verifier.successor(entryPoint->getId(), elseBlock->getId());

    // Verify convergence points
    verifier.predecessorCount(mergeBlock->getId(), 2);
    verifier.reachable(entryPoint->getId(), mergeBlock->getId());
}

TEST_F(TestCodeFlowPass, TestLowLevelLoop)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirTypeTable *typeTable = getTypeTable();
    MirOperandBuilder operandBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *loopHeader = blockBuilder.build(nullptr, "header");
    MirBlock *loopBody = blockBuilder.build(nullptr, "body");
    MirBlock *loopExit = blockBuilder.build(nullptr, "exit");

    // Fall straight through from entry to the loop evaluation header
    MirInstructionInsertionPoint entryIP{ .m_type = InsertionType::InsertAfter, .m_block = entryPoint };
    MirInstructionBuilder entryBuilder(ctx, entryIP);
    entryBuilder.JMP(operandBuilder.buildRef(loopHeader));

    // Loop Header: CMP -> JE (to exit) -> [Implicit Fallthrough to Body]
    MirRegister *counter = operandBuilder.buildVReg(typeTable->i32(), "i");
    MirInteger *limit = operandBuilder.buildInt(typeTable->i32(), FlexInt(10));

    MirInstructionInsertionPoint headerIP{ .m_type = InsertionType::InsertAfter, .m_block = loopHeader };
    MirInstructionBuilder headerBuilder(ctx, headerIP);

    headerBuilder.CMP(counter, limit);
    // Path 1 (Explicit Branch): If counter == limit, jump out of the loop
    headerBuilder.JE(operandBuilder.buildRef(loopExit));
    // Path 2 (Implicit Fallthrough): Falls into loopBody because it's next in the block list layout

    // Loop Body: Contains explicit backedge jump to loop header
    MirInstructionInsertionPoint bodyIP{ .m_type = InsertionType::InsertAfter, .m_block = loopBody };
    MirInstructionBuilder bodyBuilder(ctx, bodyIP);
    bodyBuilder.JMP(operandBuilder.buildRef(loopHeader));

    // Execute Dataflow Pipeline Verification
    CodeFlowAnalysis *pass = runPass<CodeFlowAnalysis>(ctx);
    CodeFlowAnalysisVerifier verifier(pass, ctx);

    verifier.executed().succeeded();

    // The loop header has two entries: the initial JMP from entry, and the loop body backedge
    verifier.predecessorCount(loopHeader->getId(), 2);

    // FIX: The loop header branches to loopExit (via JE) AND falls through to loopBody. Total = 2.
    verifier.successorCount(loopHeader->getId(), 2);

    verifier.successor(loopHeader->getId(), loopBody->getId());
    verifier.successor(loopHeader->getId(), loopExit->getId());

    // Structural cyclic connectivity verification
    verifier.reachable(entryPoint->getId(), loopBody->getId());
    verifier.reachable(loopBody->getId(), loopHeader->getId()); // Safe backedge loop tracking
    verifier.unreachable(loopExit->getId(), loopBody->getId());
}
