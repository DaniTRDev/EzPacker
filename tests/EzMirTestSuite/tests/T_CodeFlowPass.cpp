#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture for Control Flow Graph (CFG) analysis, reachability, and edge detection pass.
 */
class TestCodeFlowPass : public MirTestSuiteAsGtest
{
  public:
};

namespace
{

/**
 * Recursive Depth-First Search helper that traverses CFG successor edges to evaluate block reachability.
 */
bool DepthFirstSearch(CodeFlowResult *res, size_t current, size_t target, std::unordered_set<size_t> &visited)
{
    if (current == target)
        return true;
    if (visited.count(current))
        return false;

    visited.insert(current);

    auto it = res->m_successors.find(current);
    if (it == res->m_successors.end())
        return false;

    for (const auto &successor : it->second)
    {
        if (DepthFirstSearch(res, successor, target, visited))
            return true;
    }

    return false;
}

/**
 * Custom GoogleTest assertion verifying the total number of predecessor and successor map entries in the CFG result.
 */
::testing::AssertionResult HasGraphSize(CodeFlowResult *result, size_t predCount, size_t succCount)
{
    if (!result)
        return ::testing::AssertionFailure() << "Result is nullptr";

    if (result->m_predecessors.size() != predCount)
        return ::testing::AssertionFailure()
                << "Expected " << predCount << " predecessor entries, got " << result->m_predecessors.size();

    if (result->m_successors.size() != succCount)
        return ::testing::AssertionFailure()
                << "Expected " << succCount << " successor entries, got " << result->m_successors.size();

    return ::testing::AssertionSuccess();
}

/**
 * Custom GoogleTest assertion verifying that a block has 0 successors, acting as a CFG exit node.
 */
::testing::AssertionResult IsCfgExit(CodeFlowResult *result, MirBlock *exitBlock)
{
    if (!result)
        return ::testing::AssertionFailure() << "Result is nullptr";
    if (!exitBlock)
        return ::testing::AssertionFailure() << "ExitBlock is nullptr";

    size_t id = exitBlock->getId();
    auto it = result->m_successors.find(id);

    if (it == result->m_successors.end())
        return ::testing::AssertionFailure() << "Block " << id << " not found in successors map";

    if (!it->second.empty())
        return ::testing::AssertionFailure()
                << "Block " << id << " is not an exit, it has " << it->second.size() << " successors";

    return ::testing::AssertionSuccess();
}

/**
 * Custom GoogleTest assertion verifying the exact predecessor and successor count for a specific CFG node.
 */
::testing::AssertionResult IsCfgNode(CodeFlowResult *result, MirBlock *node, size_t expectedPreds, size_t expectedSuccs)
{
    if (!result)
        return ::testing::AssertionFailure() << "Result is nullptr";
    if (!node)
        return ::testing::AssertionFailure() << "Node is nullptr";

    size_t id = node->getId();
    if (!result->m_predecessors.contains(id))
        return ::testing::AssertionFailure() << "Missing predecessor entry for node " << id;
    if (!result->m_successors.contains(id))
        return ::testing::AssertionFailure() << "Missing successor entry for node " << id;

    if (result->m_predecessors[id].size() != expectedPreds)
        return ::testing::AssertionFailure() << "Node " << id << " expected " << expectedPreds << " predecessors, got "
                                             << result->m_predecessors[id].size();

    if (result->m_successors[id].size() != expectedSuccs)
        return ::testing::AssertionFailure() << "Node " << id << " expected " << expectedSuccs << " successors, got "
                                             << result->m_successors[id].size();

    return ::testing::AssertionSuccess();
}

/**
 * Custom GoogleTest assertion verifying the existence of a directed CFG edge from 'from' to 'to',
 * checking both successor and predecessor maps for bidirectional consistency.
 */
::testing::AssertionResult HasEdge(CodeFlowResult *result, MirBlock *from, MirBlock *to)
{
    if (!result)
        return ::testing::AssertionFailure() << "Result is nullptr";
    if (!from || !to)
        return ::testing::AssertionFailure() << "One or both nodes are nullptr";

    size_t fromId = from->getId();
    size_t toId = to->getId();

    // Check Successor link
    auto succIt = result->m_successors.find(fromId);
    if (succIt == result->m_successors.end() || !succIt->second.contains(toId))
        return ::testing::AssertionFailure() << "Node " << fromId << " does NOT list " << toId << " as a successor";

    // Check Predecessor link
    auto predIt = result->m_predecessors.find(toId);
    if (predIt == result->m_predecessors.end() || !predIt->second.contains(fromId))
        return ::testing::AssertionFailure() << "Node " << toId << " does NOT list " << fromId << " as a predecessor";

    return ::testing::AssertionSuccess();
}

/**
 * Custom GoogleTest assertion verifying that a directed path exists from 'from' to 'to'.
 */
::testing::AssertionResult IsReachable(CodeFlowResult *result, MirBlock *from, MirBlock *to)
{
    if (!result || !from || !to)
        return ::testing::AssertionFailure() << "Nullptr argument provided";

    std::unordered_set<size_t> visited;
    if (DepthFirstSearch(result, from->getId(), to->getId(), visited))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "No path found from block " << from->getId() << " to block " << to->getId();
}

/**
 * Custom GoogleTest assertion verifying that NO directed path exists from 'from' to 'to'.
 */
::testing::AssertionResult IsNotReachable(CodeFlowResult *result, MirBlock *from, MirBlock *to)
{
    if (!result || !from || !to)
        return ::testing::AssertionFailure() << "Nullptr argument provided";

    std::unordered_set<size_t> visited;
    if (!DepthFirstSearch(result, from->getId(), to->getId(), visited))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "Unexpected path found from block " << from->getId() << " to block "
                                         << to->getId();
}

/**
 * Helper to emit an unconditional JMP instruction from initialBlock to targetBlock.
 */
void AddCfgEdge(MirBuilderContext *context, MirBlock *initialBlock, MirBlock *targetBlock)
{
    ASSERT_NE(context, nullptr);
    MirOperandBuilder oBuilder(context);
    MirInstructionBuilder instrBuilder(context, initialBlock, InsertionType::InsertAfter, {});
    instrBuilder.JMP(oBuilder.buildRef(targetBlock));
}

/**
 * Helper to emit a conditional branch (BR_COND) with explicit true and false target block references.
 */
void AddCondEdge(MirBuilderContext *context,
                 MirRegister *condReg,
                 MirBlock *initialBlock,
                 MirBlock *trueBlock,
                 MirBlock *falseBlock)
{
    ASSERT_NE(context, nullptr);
    MirOperandBuilder oBuilder(context);
    MirInstructionBuilder instrBuilder(context, initialBlock, InsertionType::Append, {});

    // Explicitly layout both true and false paths using the new BR_COND
    instrBuilder.BR_COND(condReg, oBuilder.buildRef(trueBlock), oBuilder.buildRef(falseBlock));
}

} // anonymous namespace

/**
 * Verifies a single-block function CFG where the entry point has 0 predecessors, 0 successors,
 * and functions as the exit node.
 */
TEST_F(TestCodeFlowPass, TestFuncDoesNotHaveSuccessorsOrPredecessors)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    CodeFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 1, 1)); // Corrected to 1, 1 since the node is mapped
    EXPECT_TRUE(IsCfgExit(result, entryPoint));
}

/**
 * Verifies a linear two-block control flow graph with an unconditional edge from entry point to successor.
 */
TEST_F(TestCodeFlowPass, Test1Successor)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *successor = blockBuilder.build(nullptr, "");

    AddCfgEdge(ctx, entryPoint, successor);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    CodeFlowResult *result = pass->getResult();

    // Assert graph edge details
    EXPECT_TRUE(IsCfgNode(result, entryPoint, 0, 1));
    EXPECT_TRUE(IsCfgNode(result, successor, 1, 0));

    // Check bidirectional linkage
    EXPECT_TRUE(HasEdge(result, entryPoint, successor));

    // Evaluate global path connectivity via DFS engine
    EXPECT_TRUE(IsReachable(result, entryPoint, successor));
    EXPECT_TRUE(IsNotReachable(result, successor, entryPoint));

    EXPECT_TRUE(IsCfgExit(result, successor));
}

/**
 * Verifies diamond (if-then-else) CFG pattern containing 4 basic blocks:
 * conditional branching from entry into true/false branches that merge into a single join block.
 */
TEST_F(TestCodeFlowPass, TestDiamondPattern)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder oBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    MirBlock *trueBlock = blockBuilder.build(nullptr, "if_true");
    MirBlock *falseBlock = blockBuilder.build(nullptr, "if_false");
    MirBlock *mergeBlock = blockBuilder.build(nullptr, "merge");

    MirRegister *cond = oBuilder.buildVReg(getTypeTable()->i1(), "cond");

    // entryPoint branches to trueBlock and falseBlock conditionally via explicit target references
    AddCondEdge(ctx, cond, entryPoint, trueBlock, falseBlock);

    // Both branch paths unconditionally jump to mergeBlock
    AddCfgEdge(ctx, trueBlock, mergeBlock);
    AddCfgEdge(ctx, falseBlock, mergeBlock);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    CodeFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 4, 4));

    // Assert Node Connectivities
    EXPECT_TRUE(IsCfgNode(result, entryPoint, 0, 2));
    EXPECT_TRUE(IsCfgNode(result, trueBlock, 1, 1));
    EXPECT_TRUE(IsCfgNode(result, falseBlock, 1, 1));
    EXPECT_TRUE(IsCfgNode(result, mergeBlock, 2, 0));

    EXPECT_TRUE(HasEdge(result, entryPoint, trueBlock));
    EXPECT_TRUE(HasEdge(result, entryPoint, falseBlock));

    EXPECT_TRUE(IsReachable(result, entryPoint, mergeBlock));
    EXPECT_TRUE(IsNotReachable(result, trueBlock, falseBlock));
}

/**
 * Verifies cyclic loop control flow graph with back-edges (loop body jumping back to loop header).
 */
TEST_F(TestCodeFlowPass, TestLoopCycle)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder oBuilder(ctx);

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    MirBlock *loopHeader = blockBuilder.build(nullptr, "loop_header");
    MirBlock *loopBody = blockBuilder.build(nullptr, "loop_body");
    MirBlock *exitBlock = blockBuilder.build(nullptr, "exit");

    MirRegister *cond = oBuilder.buildVReg(getTypeTable()->i1(), "cond");

    // Enter the loop
    AddCfgEdge(ctx, entryPoint, loopHeader);

    // Conditionally continue to loopBody or break out to exitBlock
    AddCondEdge(ctx, cond, loopHeader, loopBody, exitBlock);

    // Body jumps back to header (back-edge)
    AddCfgEdge(ctx, loopBody, loopHeader);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    CodeFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 4, 4));

    EXPECT_TRUE(IsCfgNode(result, loopHeader, 2, 2)); // Preds: entry, body. Succs: body, exit
    EXPECT_TRUE(IsCfgNode(result, loopBody, 1, 1));
    EXPECT_TRUE(IsCfgNode(result, exitBlock, 1, 0));

    EXPECT_TRUE(HasEdge(result, loopBody, loopHeader));
    EXPECT_TRUE(IsReachable(result, entryPoint, exitBlock));
}

/**
 * Verifies that disconnected / dead code blocks are identified properly in CFG maps
 * without being falsely reachable from the entry point.
 */
TEST_F(TestCodeFlowPass, TestDeadCodeBlock)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    MirBlock *deadBlock = blockBuilder.build(nullptr, "dead_code");
    MirBlock *normalExit = blockBuilder.build(nullptr, "normal_exit");

    // Standard path skips deadBlock
    AddCfgEdge(ctx, entryPoint, normalExit);

    // Dead code block that accidentally jumps into the active path
    AddCfgEdge(ctx, deadBlock, normalExit);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    CodeFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 3, 3));

    EXPECT_TRUE(IsCfgNode(result, entryPoint, 0, 1));
    EXPECT_TRUE(IsCfgNode(result, deadBlock, 0, 1));
    EXPECT_TRUE(IsCfgNode(result, normalExit, 2, 0));

    EXPECT_TRUE(IsReachable(result, entryPoint, normalExit));
    EXPECT_TRUE(IsNotReachable(result, entryPoint, deadBlock));
}