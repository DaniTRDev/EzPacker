#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"

CodeFlowAnalysisPass::CodeFlowAnalysisPass(MirBuilderContext *ctx) :
    m_ctx(ctx), m_arena(ctx->getGlobalAllocator()), m_result(ctx->getGlobalAllocator())
{
}

const char *CodeFlowAnalysisPass::getName() const { return "CodeFlowAnalysisPass"; }

CodeFlowResult *CodeFlowAnalysisPass::getResult() { return &m_result; }

MirPassIterationPlace CodeFlowAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

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

        // Ensure our maps are initialized for every block, even terminal ones with zero edges
        if (!successors.contains(currentBlock->getId()))
            successors[currentBlock->getId()] = std::pmr::set<size_t>(m_arena);
        if (!predecessors.contains(currentBlock->getId()))
            predecessors[currentBlock->getId()] = std::pmr::set<size_t>(m_arena);

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

void CodeFlowAnalysisPass::reset()
{
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();
}

void CodeFlowAnalysisPass::addEdge(const MirBlock *from, const MirBlock *to)
{
    if (!from || !to)
        return;

    // Defensive check: Guard against duplicate edge tracking records inside our vectors
    auto &successors = m_result.m_successors[from->getId()];
    if (successors.find(to->getId()) == successors.end())
    {
        successors.insert(to->getId());
    }

    auto &predecessors = m_result.m_predecessors[to->getId()];
    if (predecessors.find(from->getId()) == predecessors.end())
    {
        predecessors.insert(from->getId());
    }
}