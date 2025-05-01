#include "CodeFlowGraph/CodeFlowGraph.h"

CodeFlowGraph::CodeFlowGraph() : m_firstBlock(0), m_blocks()
{
}

CodeFlowGraph::~CodeFlowGraph()
{
    m_firstBlock = 0;
    m_blocks.clear();
}

bool CodeFlowGraph::doesBlockExist(uint64_t address) const
{
    return m_blocks.contains(address);
}

bool CodeFlowGraph::isBlockChildOf(CodeFlowBlock *parent, uint64_t block) const
{
    if (parent == nullptr)
        return false;

    return parent->m_children.contains(block);
}

bool CodeFlowGraph::pushChildrenIfExist(uint64_t from, uint64_t to)
{
    CodeFlowBlock *fromBlock = getBlock(from);
    CodeFlowBlock *toBlock = getBlock(to);

    if (fromBlock == nullptr || toBlock == nullptr || isBlockChildOf(fromBlock, to))
        return false;

    fromBlock->m_children.insert(to);
    return true;
}

bool CodeFlowGraph::pushIfNotExists(CodeFlowBlockType type, uint64_t address)
{
    if (doesBlockExist(address))
        return false;

    if (m_blocks.size() == 0)
    {
        // First block. Save its address.
        m_firstBlock = address;
    }

    m_blocks.insert({address, std::make_shared<CodeFlowBlock>(
                                  CodeFlowBlock{.m_type = type, .m_address = address, .m_children = {}})});
    return true;
}

CodeFlowBlock *CodeFlowGraph::getBlock(uint64_t address) const
{
    if (!m_blocks.contains(address))
        return nullptr;

    return m_blocks.at(address).get();
}

void CodeFlowGraph::printGraph() const
{
    std::set<uint64_t> visitedBlocks;
    std::stack<CodeFlowBlock *> remainingBlocks;

    remainingBlocks.push(getBlock(m_firstBlock));
    while (!remainingBlocks.empty())
    {
        CodeFlowBlock *block = remainingBlocks.top();
        remainingBlocks.pop(); // Get top block to visit.

        // Mark as visited.
        visitedBlocks.insert(block->m_address);
        
        std::cout << std::hex << block->m_address << ", " << CodeFlowBlockType2Str[block->m_type] << " -> ["
                  << std::flush;
        
        size_t childCounter = 0;
        for (auto &child : block->m_children)
        {
            std::cout << std::hex << child << std::flush;

            if (!visitedBlocks.contains(child))        // If not visited already, put this block in the visiting list.
                remainingBlocks.push(getBlock(child)); // Push children to the visiting list.

            childCounter++;

            if (childCounter != block->m_children.size())
                std::cout << ", " << std::flush;
        }

        std::cout << "]" << std::endl;
    }
}

std::vector<CodeFlowBlock *> CodeFlowGraph::getBlockParents(uint64_t address) const
{
    std::vector<CodeFlowBlock *> result;
    for (auto &[blockAddress, block] : m_blocks)
    {
        if (isBlockChildOf(block.get(), address))
            result.push_back(block.get());
    }

    return result;
}
