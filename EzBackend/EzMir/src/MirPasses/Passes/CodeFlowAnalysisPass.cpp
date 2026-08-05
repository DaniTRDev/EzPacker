#include "MirPasses/Passes/CodeFlowAnalysisPass.h"

CodeFlowAnalysisPass::CodeFlowAnalysisPass(MirBuilderContext *ctx) :
    m_ctx(ctx), m_arena(ctx->getGlobalAllocator()), m_result(ctx->getGlobalAllocator())
{
}

const char *CodeFlowAnalysisPass::getName() const { return "CodeFlowAnalysisPass"; }

const ControlFlowResult &CodeFlowAnalysisPass::getResult() const { return m_result; }

MirPassIterationPlace CodeFlowAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult CodeFlowAnalysisPass::run(std::pmr::list<MirFunction *> &funcList,
                                    std::pmr::list<struct MirFunction *>::iterator it,
                                    class MirPassManager *passManager)
{
    MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();
    {
        auto log = diag->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("Computing Control Flow Graph (CFG) topology for: '{}'", func->getName()));
    }

    auto &blockList = func->getBlocks();

    for (auto blockIt = blockList.begin(); blockIt != blockList.end(); ++blockIt)
    {
        MirBlock *currentBlock = *blockIt;

        // Ensure our maps are initialized for every block, even terminal ones with zero edges
        if (!m_result.m_successors.contains(currentBlock->getId()))
            m_result.m_successors[currentBlock->getId()] = std::pmr::set<size_t>(m_arena);
        if (!m_result.m_predecessors.contains(currentBlock->getId()))
            m_result.m_predecessors[currentBlock->getId()] = std::pmr::set<size_t>(m_arena);

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
                MirBlock *jumpTarget = getTargetJumpBlock(inst);

                if (jumpTarget)
                {
                    addEdge(currentBlock, jumpTarget);
                }
                else
                {
                    auto log = diag->builder(DiagnosticMessageType::Diag_Warning, getName());
                    log << "Branch instruction lacks a valid Target Basic Block reference layout frame.";
                }

                // If it's an unconditional branch (does NOT read CPU flags), no subsequent
                // instructions in this block can execute.
                if ((flags & MirInstructionFlags::ReadsCPUFlags) == 0)
                {
                    hasUnconditionalJump = true;
                    break;
                }
            }
            else if (flags & MirInstructionFlags::IsReturn)
            {
                isReturnBlock = true;

                auto log = diag->builder(DiagnosticMessageType::Diag_Debug, getName());
                log << std::pmr::string(
                        std::format("Found leaf node function exit terminal at Block ID: {}", currentBlock->getId()));

                break; // Return statements instantly terminate block evaluation
            }
        }

        // Natural code fallthrough logic:
        // If the block didn't end with an unconditional jump or a return statement,
        // it falls through to the next sequential block in memory layout.
        if (!hasUnconditionalJump && !isReturnBlock)
        {
            // Note: If the block had a conditional branch, the 'true' path was handled
            // inside the loop above, and this adds the 'false' (fallthrough) path.
            if (nextBlock)
            {
                addEdge(currentBlock, nextBlock);
            }
        }
    }

    // Analysis passes never mutate bytecode layouts
    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

void CodeFlowAnalysisPass::printResult() const
{
    const auto &result = getResult();
    auto diag = m_ctx->getDiagCollector();

    {
        auto log = diag->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("CodeFlowAnalysisPass SUCCESSOR list:"));

        for (auto &[blockId, successors] : result.m_successors)
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

            log.appendNote(succeededBy.data(), nullptr);
        }
    }

    {
        auto log = diag->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("CodeFlowAnalysisPass PREDECESSOR list:"));

        for (auto &[blockId, predecessors] : result.m_predecessors)
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

            log.appendNote(precededBy.c_str(), nullptr);
        }
    }
}

void CodeFlowAnalysisPass::reset()
{
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();
}

MirBlock *CodeFlowAnalysisPass::getTargetJumpBlock(const MirInstruction *inst) const
{
    if (!inst || inst->getOperands().empty())
        return nullptr;

    // Secure the target tracking operand index block securely
    const MirOperand *op = inst->getOperands().front();
    if (!op)
        return nullptr;

    const MirReference *ref = op->get<MirReference>();
    if (!ref)
        return nullptr;

    return m_ctx->getBlockById(ref->getRefId());
}

void CodeFlowAnalysisPass::addEdge(MirBlock *from, MirBlock *to)
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
