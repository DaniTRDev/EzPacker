#include "CodeFlowPassVerifier.h"

CodeFlowAnalysisVerifier::CodeFlowAnalysisVerifier(CodeFlowAnalysis *analysis, MirBuilderContext *ctx) :
    m_ctx(ctx), MirPassVerifier(analysis)
{
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::predecessor(size_t toId, size_t fromId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_predecessors.find(toId);

    EXPECT_TRUE(res.m_predecessors.contains(fromId));
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::predecessorCount(size_t blockId, size_t count)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_predecessors.find(blockId);

    EXPECT_NE(it, res.m_predecessors.end());
    EXPECT_EQ(it->second.size(), count);

    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::reachable(size_t start, size_t end)
{
    const auto &res = getTestedObj()->getResult();

    // Lambda for Depth-First Search traversal
    // We use a tracking set passed by reference to handle cycles safely
    std::function<bool(size_t, size_t, std::unordered_set<size_t> &)> dfs =
            [&res, &dfs](size_t current, size_t target, std::unordered_set<size_t> &visited) -> bool
    {
        // We found our target destination block
        if (current == target)
            return true;

        // We hit a block we've already evaluated (prevents infinite loop in cycles)
        if (visited.count(current))
            return false;

        // Mark current block as processed
        visited.insert(current);

        // Look up successors for the current block
        auto it = res.m_successors.find(current);
        if (it == res.m_successors.end())
            return false; // Dead end / Sink block

        // Recursively check all outgoing control flow branches
        for (const auto &successor : it->second)
        {
            // If any path leads to the target block, cascade a success back up
            if (dfs(successor, target, visited))
                return true;
        }

        return false;
    };

    std::unordered_set<size_t> visited;
    bool isReachable = dfs(start, end, visited);

    EXPECT_TRUE(isReachable) << "Block " << end << " is expected to be reachable from Block " << start
                             << ", but no continuous path was found.";

    return *this;
}
CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::successor(size_t fromId, size_t toId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(fromId);

    EXPECT_TRUE(it->second.contains(toId));
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::successorCount(size_t blockId, size_t count)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(blockId);

    EXPECT_NE(it, res.m_successors.end());
    EXPECT_EQ(it->second.size(), count);

    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::unreachable(size_t start, size_t end)
{
    const auto &res = getTestedObj()->getResult();

    // Lambda for Depth-First Search traversal
    // We use a tracking set passed by reference to handle cycles safely
    std::function<bool(size_t, size_t, std::unordered_set<size_t> &)> dfs =
            [&res, &dfs](size_t current, size_t target, std::unordered_set<size_t> &visited) -> bool
    {
        // We found our target destination block
        if (current == target)
            return true;

        // We hit a block we've already evaluated (prevents infinite loop in cycles)
        if (visited.count(current))
            return false;

        // Mark current block as processed
        visited.insert(current);

        // Look up successors for the current block
        auto it = res.m_successors.find(current);
        if (it == res.m_successors.end())
            return false; // Dead end / Sink block

        // Recursively check all outgoing control flow branches
        for (const auto &successor : it->second)
        {
            // If any path leads to the target block, cascade a success back up
            if (dfs(successor, target, visited))
                return true;
        }

        return false;
    };

    std::unordered_set<size_t> visited;
    bool isReachable = dfs(start, end, visited);

    EXPECT_FALSE(isReachable) << "Block " << end << " is expected to be unreachable from Block " << start;
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::exitBlock(size_t blockId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(blockId);

    EXPECT_NE(it, res.m_successors.end());
    EXPECT_EQ(it->second.size(), 0);

    return *this;
}
