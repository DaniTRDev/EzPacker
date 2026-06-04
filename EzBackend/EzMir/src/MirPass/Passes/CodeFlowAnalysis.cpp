#include "MirPass/Passes/CodeFlowAnalysis.h"

CodeFlowAnalysis::CodeFlowAnalysis(MirBuilderContext *ctx) :
    m_ctx(ctx), m_arena(ctx->getGlobalAllocator()), m_result(ctx->getGlobalAllocator())
{
}

const char *CodeFlowAnalysis::getName() const { return "CodeFlowAnalysis"; }

const ControlFlowResult &CodeFlowAnalysis::getResult() const { return m_result; }

MirPassIterationPlace CodeFlowAnalysis::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult CodeFlowAnalysis::run(std::pmr::list<MirFunction *> &funcList,
                                    std::pmr::list<struct MirFunction *>::iterator it,
                                    class MirPassManager *passManager)
{
    MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();
    {
        auto log = diag->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("Computing Control Flow Graph (CFG) topology for: '{}'", func->getName()));
    }

    // Reset old tracking arrays safely
    m_result.m_successors.clear();
    m_result.m_predecessors.clear();

    auto &blockList = func->getBlocks();

    for (auto blockIt = blockList.begin(); blockIt != blockList.end(); ++blockIt)
    {
        MirBlock *currentBlock = *blockIt;

        // Ensure our maps are initialized for every block, even terminal ones with zero edges
        if (!m_result.m_successors.contains(currentBlock))
            m_result.m_successors[currentBlock] = std::pmr::vector<MirBlock *>(m_arena);
        if (!m_result.m_predecessors.contains(currentBlock))
            m_result.m_predecessors[currentBlock] = std::pmr::vector<MirBlock *>(m_arena);

        const auto &instructions = currentBlock->getInstructions();
        const MirInstruction *terminator = nullptr;
        MirInstructionFlags terminatorFlags = MirInstructionFlags::None;

        if (!instructions.empty())
        {
            terminator = instructions.back();
            terminatorFlags = terminator->getFlags();
        }

        // Identify fallback path coordinates
        auto nextIt = blockIt;
        ++nextIt;
        MirBlock *nextBlock = (nextIt != blockList.end()) ? (*nextIt) : nullptr;

        // Process Graph Topologies depending on instruction termination rules
        if (terminator && (static_cast<uint8_t>(terminatorFlags) & static_cast<uint8_t>(MirInstructionFlags::IsBranch)))
        {
            MirBlock *jumpTarget = getTargetJumpBlock(terminator);

            if ((static_cast<uint8_t>(terminatorFlags) & static_cast<uint8_t>(MirInstructionFlags::ReadsCPUFlags)) == 0)
            {
                // Unconditional Jump Edge
                if (jumpTarget)
                {
                    addEdge(currentBlock, jumpTarget);
                }
                else
                {
                    auto log = diag->builder(DiagnosticMessageType::Diag_Warning, getName());
                    log << "Unconditional branch lacks a valid Target Basic Block reference layout frame.";
                }
            }
            else
            {
                // Conditional Jump Edge (Maps BOTH target path and fallback natural line)
                if (jumpTarget)
                    addEdge(currentBlock, jumpTarget);
                if (nextBlock)
                    addEdge(currentBlock, nextBlock);
            }
        }
        else if (terminator &&
                 (static_cast<uint8_t>(terminatorFlags) & static_cast<uint8_t>(MirInstructionFlags::IsReturn)))
        {
            // Terminal block - explicitly consumes code tracking tracks cleanly
            {
                auto log = diag->builder(DiagnosticMessageType::Diag_Debug, getName());
                log << std::pmr::string(
                        std::format("Found leaf node function exit terminal at Block ID: {}", currentBlock->getId()));
            }
        }
        else
        {
            // Natural code fallthrough sequence execution
            if (nextBlock)
            {
                addEdge(currentBlock, nextBlock);
            }
        }
    }

    // Analysis passes never mutate bytecode layouts
    return { .m_modifiedMir = false, .m_run = true, .m_succeeded = true };
}

MirBlock *CodeFlowAnalysis::getTargetJumpBlock(const MirInstruction *inst) const
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

void CodeFlowAnalysis::addEdge(MirBlock *from, MirBlock *to)
{
    if (!from || !to)
        return;

    // Defensive check: Guard against duplicate edge tracking records inside our vectors
    auto &successors = m_result.m_successors[from];
    if (std::find(successors.begin(), successors.end(), to) == successors.end())
    {
        successors.push_back(to);
    }

    auto &predecessors = m_result.m_predecessors[to];
    if (std::find(predecessors.begin(), predecessors.end(), from) == predecessors.end())
    {
        predecessors.push_back(from);
    }
}
