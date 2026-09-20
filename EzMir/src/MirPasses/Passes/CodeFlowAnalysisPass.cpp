#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"
#include <algorithm>
#include <utility>

namespace
{

/**
 * Inserts id into a sorted MirId list, keeping the list unique. Adjacency lists are tiny, so a
 * lower_bound + insert is cheaper than the ordered set it replaces.
 */
void insertSortedUnique(std::pmr::vector<MirId> &list, MirId id)
{
    auto it = std::lower_bound(list.begin(), list.end(), id);
    if (it == list.end() || *it != id)
    {
        list.insert(it, id);
    }
}

} // namespace

/**
 * Initializes the result storage using the supplied arena.
 */
CodeFlowResult::CodeFlowResult(std::pmr::memory_resource *arena) :
    m_arena(arena), m_blockIds(arena), m_successors(arena), m_predecessors(arena), m_slotById(arena)
{
}

/**
 * Assigns a compact slot to blockId on first sight and returns it.
 */
size_t CodeFlowResult::ensureBlock(MirId blockId)
{
    auto [it, inserted] = m_slotById.try_emplace(blockId, m_blockIds.size());
    if (inserted)
    {
        m_blockIds.push_back(blockId);
        m_successors.push_back(std::pmr::vector<MirId>(m_arena));
        m_predecessors.push_back(std::pmr::vector<MirId>(m_arena));
    }
    return it->second;
}

/**
 * Records an edge in both directions, de-duplicating and preserving MirId ordering.
 */
void CodeFlowResult::addEdge(MirId from, MirId to)
{
    const size_t fromSlot = ensureBlock(from);
    const size_t toSlot = ensureBlock(to);
    insertSortedUnique(m_successors[fromSlot], to);
    insertSortedUnique(m_predecessors[toSlot], from);
}

/**
 * Returns true when blockId has an assigned slot.
 */
bool CodeFlowResult::contains(MirId blockId) const { return m_slotById.find(blockId) != m_slotById.end(); }

/**
 * Returns the number of tracked blocks.
 */
size_t CodeFlowResult::getBlockCount() const { return m_blockIds.size(); }

/**
 * Returns the slot for blockId or InvalidSlot when absent.
 */
size_t CodeFlowResult::slotOf(MirId blockId) const
{
    auto it = m_slotById.find(blockId);
    return it != m_slotById.end() ? it->second : InvalidSlot;
}

/**
 * Returns the successor list of blockId, or an empty span when the block is unknown.
 */
std::span<const MirId> CodeFlowResult::getSuccessors(MirId blockId) const
{
    const size_t slot = slotOf(blockId);
    return slot == InvalidSlot ? std::span<const MirId>{} : std::span<const MirId>{ m_successors[slot] };
}

/**
 * Returns the predecessor list of blockId, or an empty span when the block is unknown.
 */
std::span<const MirId> CodeFlowResult::getPredecessors(MirId blockId) const
{
    const size_t slot = slotOf(blockId);
    return slot == InvalidSlot ? std::span<const MirId>{} : std::span<const MirId>{ m_predecessors[slot] };
}

/**
 * Returns the block IDs in slot order.
 */
const std::pmr::vector<MirId> &CodeFlowResult::getBlockIds() const { return m_blockIds; }

/**
 * Clears every block slot and edge so the graph can be rebuilt for another module.
 */
void CodeFlowResult::reset()
{
    m_blockIds.clear();
    m_successors.clear();
    m_predecessors.clear();
    m_slotById.clear();
}

/**
 * Initializes the CFG pass and its result storage using the context's global arena.
 */
CodeFlowAnalysisPass::CodeFlowAnalysisPass(MirBuilderContext *ctx) :
    m_result(ctx->getGlobalAllocator()), m_ctx(ctx), m_arena(ctx->getGlobalAllocator())
{
}

/**
 * Returns the pass identifier.
 */
const char *CodeFlowAnalysisPass::getName() const { return "CodeFlowAnalysisPass"; }

/**
 * Returns the computed CFG adjacency mappings.
 */
CodeFlowResult *CodeFlowAnalysisPass::getResult() { return &m_result; }

/**
 * Runs once per function.
 */
MirPassIterationPlace CodeFlowAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

/**
 * Builds the CFG for the target function: initializes a slot per block, adds edges for branch-target
 * operands, and adds the implicit fallthrough edge when a block neither ends in an unconditional
 * branch nor a return.
 */
MirPassResult CodeFlowAnalysisPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                        class MirPassManager *passManager)
{
    MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();

    // Direct lazy formatting without eager allocations
    diag->trace(getName(), "Computing Control Flow Graph (CFG) topology for: '{}'", func->getName());

    const auto &blockList = func->getBlocks();

    for (auto blockIt = blockList.begin(); blockIt != blockList.end(); ++blockIt)
    {
        const MirBlock *currentBlock = *blockIt;

        // Ensure a slot exists for every block, even terminal ones with zero edges.
        m_result.ensureBlock(currentBlock->getId());

        // Identify fallback path coordinates (the next sequential block in code layout)
        auto nextIt = blockIt;
        ++nextIt;
        MirBlock *nextBlock = (nextIt != blockList.end()) ? (*nextIt) : nullptr;

        const auto &instructions = currentBlock->getInstructions();
        bool hasUnconditionalJump = false;
        bool isReturnBlock = false;

        // Scan through all instructions to process multiple branches inside the same basic block
        for (const MirInstruction *inst : instructions)
        {
            MirInstructionFlags flags = inst->getFlags();
            if (flags & MirInstructionFlags::IsBranch)
            {
                bool foundAnyTarget = false;

                for (const MirOperand *op : inst->getOperands())
                {
                    if (op && op->getType() == MirOperandType::Reference)
                    {
                        if (const MirReference *ref = op->get<MirReference>())
                        {
                            // If the reference is a function (e.g., in a CALL), getBlockById gracefully returns nullptr
                            MirBlock *jumpTarget = m_ctx->getBlockById(ref->getRefId());
                            if (jumpTarget)
                            {
                                addEdge(currentBlock, jumpTarget);
                                foundAnyTarget = true;
                            }
                        }
                    }
                }

                if (!foundAnyTarget)
                {
                    auto log = diag->builder(DiagnosticMessageType::Diag_Warning, getName());
                    log << "Branch instruction lacks a valid Target Basic Block reference layout frame.";
                }

                // If it's an unconditional branch (does NOT read CPU flags), no subsequent
                // instructions in this block can execute.
                // Note: BR_COND explicitly maps BOTH true and false blocks as operands, so it
                // satisfies the CFG topology exclusively via its operands and does not need fallthrough.
                if ((flags & MirInstructionFlags::ReadsCPUFlags) == 0)
                {
                    hasUnconditionalJump = true;
                    break;
                }
            }
            else if (flags & MirInstructionFlags::IsReturn)
            {
                isReturnBlock = true;

                diag->trace(getName(), "Found leaf node function exit terminal at Block ID: {}", currentBlock->getId())
                        << inst->getSourceRef();

                break; // Return statements instantly terminate block evaluation
            }
        }

        // Natural code fallthrough logic:
        // If the block didn't end with an unconditional jump or a return statement,
        // it falls through to the next sequential block in memory layout.
        if (!hasUnconditionalJump && !isReturnBlock)
        {
            if (nextBlock)
            {
                addEdge(currentBlock, nextBlock);
            }
        }
    }

    // Publish the ordered predecessor lists as explicit block metadata. Later stages (notably PHI
    // selection) consume the same ordering NonSsaToSsaPass uses to fill incoming operands, so they
    // never have to reconstruct the CFG by scanning instruction operands themselves.
    for (MirBlock *block : func->getBlocks())
    {
        std::pmr::vector<MirBlock *> preds(m_arena);
        const std::span<const MirId> predIds = m_result.getPredecessors(block->getId());
        preds.reserve(predIds.size());
        for (MirId predId : predIds)
        {
            if (MirBlock *predBlock = m_ctx->getBlockById(predId))
            {
                preds.push_back(predBlock);
            }
        }
        block->setPredecessors(std::move(preds));
    }

    // Analysis passes never mutate bytecode layouts
    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

/**
 * Traces the successor and predecessor list of every block; skips all work when trace diagnostics
 * are disabled.
 */
void CodeFlowAnalysisPass::printResult()
{
    auto diag = m_ctx->getDiagCollector();

    // Guard the heavy string constructions and print loops entirely
    if (!diag->isDiagEnabledForType(DiagnosticMessageType::Diag_Trace))
        return;

    auto result = getResult();

    {
        auto log = diag->trace(getName(), "CodeFlowAnalysisPass SUCCESSOR list:");

        for (MirId blockId : result->getBlockIds())
        {
            std::string succeededBy =
                    MirPrinter::printToString(m_ctx->getBlockById(blockId), MirPrinterDetail::General);

            for (MirId successor : result->getSuccessors(blockId))
            {
                succeededBy += MirPrinter::printToString(m_ctx->getBlockById(successor), MirPrinterDetail::General);
            }

            if (result->getSuccessors(blockId).empty())
            {
                succeededBy += "empty\n";
            }

            log.appendNote("{}", succeededBy);
        }
    }

    {
        auto log = diag->trace(getName(), "CodeFlowAnalysisPass PREDECESSOR list:");

        for (MirId blockId : result->getBlockIds())
        {
            std::string precededBy = MirPrinter::printToString(m_ctx->getBlockById(blockId), MirPrinterDetail::General);
            for (MirId predecessor : result->getPredecessors(blockId))
            {
                precededBy += MirPrinter::printToString(m_ctx->getBlockById(predecessor), MirPrinterDetail::General);
            }

            if (result->getPredecessors(blockId).empty())
            {
                precededBy += "empty\n";
            }

            log.appendNote("{}", precededBy);
        }
    }
}

/**
 * Clears the previous successor/predecessor mappings so the pass can run on another function.
 */
void CodeFlowAnalysisPass::reset() { m_result.reset(); }

/**
 * Records a directed edge from -> to, ignoring null blocks.
 */
void CodeFlowAnalysisPass::addEdge(const MirBlock *from, const MirBlock *to)
{
    if (!from || !to)
        return;

    m_result.addEdge(from->getId(), to->getId());
}
