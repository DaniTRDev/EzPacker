#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

class TestCodeFlowPass : public MirTestSuiteAsGtest
{
  public:
};

namespace
{

bool DepthFirstSearch(ControlFlowResult *res, size_t current, size_t target, std::unordered_set<size_t> &visited)
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

::testing::AssertionResult HasGraphSize(ControlFlowResult *result, size_t predCount, size_t succCount)
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

::testing::AssertionResult IsCfgExit(ControlFlowResult *result, MirBlock *exitBlock)
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

::testing::AssertionResult
IsCfgNode(ControlFlowResult *result, MirBlock *node, size_t expectedPreds, size_t expectedSuccs)
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

::testing::AssertionResult HasEdge(ControlFlowResult *result, MirBlock *from, MirBlock *to)
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

::testing::AssertionResult IsReachable(ControlFlowResult *result, MirBlock *from, MirBlock *to)
{
    if (!result || !from || !to)
        return ::testing::AssertionFailure() << "Nullptr argument provided";

    std::unordered_set<size_t> visited;
    if (DepthFirstSearch(result, from->getId(), to->getId(), visited))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "No path found from block " << from->getId() << " to block " << to->getId();
}

::testing::AssertionResult IsNotReachable(ControlFlowResult *result, MirBlock *from, MirBlock *to)
{
    if (!result || !from || !to)
        return ::testing::AssertionFailure() << "Nullptr argument provided";

    std::unordered_set<size_t> visited;
    if (!DepthFirstSearch(result, from->getId(), to->getId(), visited))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "Unexpected path found from block " << from->getId() << " to block "
                                         << to->getId();
}

void AddCfgEdge(MirBuilderContext *context, MirBlock *initialBlock, MirBlock *targetBlock)
{
    ASSERT_NE(context, nullptr);
    MirOperandBuilder oBuilder(context);
    MirInstructionBuilder instrBuilder(context, initialBlock, InsertionType::InsertAfter, initialBlock->begin());
    instrBuilder.JMP(oBuilder.buildRef(targetBlock));
}

void AddCondEdge(MirBuilderContext *context, MirBlock *initialBlock, MirBlock *targetBlock)
{
    ASSERT_NE(context, nullptr);
    MirOperandBuilder oBuilder(context);
    MirInstructionBuilder instrBuilder(context, initialBlock, InsertionType::InsertAfter, initialBlock->begin());
    // Uses a conditional jump so the block can have 2 successors (this jump + the natural fallthrough)
    instrBuilder.JE(oBuilder.buildRef(targetBlock));
}

} // anonymous namespace

TEST_F(TestCodeFlowPass, TestFuncDoesNotHaveSuccessorsOrPredecessors)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    ControlFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 1, 1)); // Corrected to 1, 1 since the node is mapped
    EXPECT_TRUE(IsCfgExit(result, entryPoint));
}

TEST_F(TestCodeFlowPass, Test1Successor)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *successor = blockBuilder.build(nullptr, "");

    AddCfgEdge(ctx, entryPoint, successor);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    ControlFlowResult *result = pass->getResult();

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

TEST_F(TestCodeFlowPass, TestDiamondPattern)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    // Layout order is important for fallthrough: entryPoint -> trueBlock -> falseBlock -> mergeBlock
    MirBlock *trueBlock = blockBuilder.build(nullptr, "if_true");
    MirBlock *falseBlock = blockBuilder.build(nullptr, "if_false");
    MirBlock *mergeBlock = blockBuilder.build(nullptr, "merge");

    // entryPoint branches to falseBlock conditionally.
    // If condition fails, it natively falls through to the next block in layout (trueBlock).
    AddCondEdge(ctx, entryPoint, falseBlock);

    // Both branch paths jump to mergeBlock
    AddCfgEdge(ctx, trueBlock, mergeBlock);
    AddCfgEdge(ctx, falseBlock, mergeBlock);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    ControlFlowResult *result = pass->getResult();

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

TEST_F(TestCodeFlowPass, TestLoopCycle)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    // Layout order: entry -> header -> body -> exit
    MirBlock *loopHeader = blockBuilder.build(nullptr, "loop_header");
    MirBlock *loopBody = blockBuilder.build(nullptr, "loop_body");
    MirBlock *exitBlock = blockBuilder.build(nullptr, "exit");

    // Enter the loop
    AddCfgEdge(ctx, entryPoint, loopHeader);

    // Conditionally break out to exitBlock. If not taken, falls through to loopBody
    AddCondEdge(ctx, loopHeader, exitBlock);

    // Body jumps back to header (back-edge)
    AddCfgEdge(ctx, loopBody, loopHeader);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    ControlFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 4, 4));

    EXPECT_TRUE(IsCfgNode(result, loopHeader, 2, 2)); // Preds: entry, body. Succs: body, exit
    EXPECT_TRUE(IsCfgNode(result, loopBody, 1, 1));
    EXPECT_TRUE(IsCfgNode(result, exitBlock, 1, 0));

    EXPECT_TRUE(HasEdge(result, loopBody, loopHeader));
    EXPECT_TRUE(IsReachable(result, entryPoint, exitBlock));
}

TEST_F(TestCodeFlowPass, TestDeadCodeBlock)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();

    // IMPORTANT FIX: Create deadBlock BEFORE normalExit.
    // Because normalExit has no terminal instruction, it will try to fallthrough
    // to whatever is physically next. Putting it last prevents it from falling into dead code.
    MirBlock *deadBlock = blockBuilder.build(nullptr, "dead_code");
    MirBlock *normalExit = blockBuilder.build(nullptr, "normal_exit");

    // Standard path skips deadBlock
    AddCfgEdge(ctx, entryPoint, normalExit);

    // Dead code block that accidentally jumps into the active path
    AddCfgEdge(ctx, deadBlock, normalExit);

    CodeFlowAnalysisPass *pass = runPass<CodeFlowAnalysisPass>(ctx);
    ControlFlowResult *result = pass->getResult();

    EXPECT_TRUE(HasGraphSize(result, 3, 3));

    EXPECT_TRUE(IsCfgNode(result, entryPoint, 0, 1));
    EXPECT_TRUE(IsCfgNode(result, deadBlock, 0, 1));
    EXPECT_TRUE(IsCfgNode(result, normalExit, 2, 0));

    EXPECT_TRUE(IsReachable(result, entryPoint, normalExit));
    EXPECT_TRUE(IsNotReachable(result, entryPoint, deadBlock));
}