#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"

/**
 * Initializes the CFG pass and its result storage using the context's global arena.
 */
CodeFlowAnalysisPass::CodeFlowAnalysisPass(MirBuilderContext *ctx) :
    m_ctx(ctx), m_arena(ctx->getGlobalAllocator()), m_result(ctx->getGlobalAllocator())
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
 * Builds the CFG for the target function: initializes an adjacency entry per block, adds edges
 * for branch-target operands, and adds the implicit fallthrough edge when a block neither ends in
 * an unconditional branch nor a return.
 */
MirPassResult CodeFlowAnalysisPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                        class MirPassManager *passManager)
{
    const MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();

    // Direct lazy formatting without eager allocations
    diag->trace(getName(), "Computing Control Flow Graph (CFG) topology for: '{}'", func->getName());

    const auto &blockList = func->getBlocks();
    auto &predecessors = m_result.m_predecessors;
    auto &successors = m_result.m_successors;

    for (auto blockIt = blockList.begin(); blockIt != blockList.end(); ++blockIt)
    {
        const MirBlock *currentBlock = *blockIt;

        // Ensure our maps are initialized for every block, even terminal ones with zero edges.
        // insert() de-duplicates, so a single lookup suffices and the set keeps the arena.
        successors.insert({ currentBlock->getId(), std::pmr::set<MirId>(m_arena) });
        predecessors.insert({ currentBlock->getId(), std::pmr::set<MirId>(m_arena) });

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

        for (auto &[blockId, successors] : result->m_successors)
        {
            std::string succeededBy =
                    MirPrinter::printToString(m_ctx->getBlockById(blockId), MirPrinterDetail::General);

            for (auto &successor : successors)
            {
                succeededBy += MirPrinter::printToString(m_ctx->getBlockById(successor), MirPrinterDetail::General);
            }

            if (successors.empty())
            {
                succeededBy += "empty\n";
            }

            log.appendNote("{}", succeededBy);
        }
    }

    {
        auto log = diag->trace(getName(), "CodeFlowAnalysisPass PREDECESSOR list:");

        for (auto &[blockId, predecessors] : result->m_predecessors)
        {
            std::string precededBy = MirPrinter::printToString(m_ctx->getBlockById(blockId), MirPrinterDetail::General);
            for (auto &predecessor : predecessors)
            {
                precededBy += MirPrinter::printToString(m_ctx->getBlockById(predecessor), MirPrinterDetail::General);
            }

            if (predecessors.empty())
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
void CodeFlowAnalysisPass::reset()
{
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();
}

/**
 * Records a directed edge from -> to in both the successor and predecessor sets, ignoring null
 * blocks and duplicate edges.
 */
void CodeFlowAnalysisPass::addEdge(const MirBlock *from, const MirBlock *to)
{
    if (!from || !to)
        return;

    // set::insert ignores duplicates, so a single insertion both adds the edge and de-duplicates it.
    m_result.m_successors[from->getId()].insert(to->getId());
    m_result.m_predecessors[to->getId()].insert(from->getId());
}