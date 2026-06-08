#include "MirPasses/Passes/LivenessAnalysis.h"

static std::string formatRegisterSet(MirBuilderContext *ctx, const std::pmr::unordered_set<size_t> &regSet)
{
    if (regSet.empty())
        return "{}";

    std::ostringstream ss;
    ss << "{ ";
    bool first = true;
    for (size_t regId : regSet)
    {
        MirRegister *reg = ctx->getRegisterById(regId);

        if (!first)
            ss << ", ";
        // Format virtual registers as %v0, %v1 and physical ones as %r0, %p1

        ss << MirPrinter::printToString(reg);

        first = false;
    }
    ss << " }";
    return ss.str();
}

LivenessAnalysis::LivenessAnalysis(MirBuilderContext *ctx) :
    m_result(ctx->getGlobalAllocator()), m_ctx(ctx), m_arena(ctx->getGlobalAllocator())
{
}

const char *LivenessAnalysis::getName() const { return "LivenessAnalysis"; }

const LivenessResult &LivenessAnalysis::getResult() const { return m_result; }

MirPassIterationPlace LivenessAnalysis::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult LivenessAnalysis::run(std::pmr::list<MirFunction *> &funcList,
                                    std::pmr::list<MirFunction *>::iterator it,
                                    class MirPassManager *passManager)
{
    MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();
    {
        auto log = diag->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("Analyzing register liveness spans for function: '{}'", func->getName()));
    }

    // Recover the pre-computed Control Flow Graph directly from the Pass Manager cache
    const auto &cfg = passManager->getAnalysis<CodeFlowAnalysis>(funcList)->getResult();

    // Initialize and extract block-local Gen (Use) and Kill (Def) sets
    computeLocalLiveness(func, diag);

    // Solve global fixed-point backward equations across our CFG topology paths
    computeGlobalLiveness(func, cfg);

    auto log = diag->builder(DiagnosticMessageType::Diag_Debug, getName());
    log << std::pmr::string(std::format("Final Liveness Analysis Matrix for Function '{}':", func->getName()));

    for (auto &block : func->getBlocks())
    {
        size_t blockId = block->getId();
        log.appendNote(std::pmr::string(std::format("Block ID {}:", block->getId())), nullptr);

        log.appendNote(
                std::pmr::string(std::format("  Local  DEF: {}", formatRegisterSet(m_ctx, m_result.m_def[blockId]))),
                nullptr);

        log.appendNote(
                std::pmr::string(std::format("  Local  USE: {}", formatRegisterSet(m_ctx, m_result.m_use[blockId]))),
                nullptr);

        log.appendNote(std::pmr::string(std::format("  Global LIVE-IN:  {}",
                                                    formatRegisterSet(m_ctx, m_result.m_liveIn[blockId]))),
                       nullptr);

        log.appendNote(std::pmr::string(std::format("  Global LIVE-OUT: {}",
                                                    formatRegisterSet(m_ctx, m_result.m_liveOut[blockId]))),
                       nullptr);
    }

    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

std::vector<std::type_index> LivenessAnalysis::getDependencies() const
{
    return { std::type_index(typeid(CodeFlowAnalysis)) };
}

void LivenessAnalysis::computeGlobalLiveness(MirFunction *func, const ControlFlowResult &cfg)
{
    auto &blocks = func->getBlocks();
    bool changed = true;
    size_t iterations = 0;

    // Fixed-point solver loop runs until data propagates completely and sets stabilize
    while (changed)
    {
        changed = false;
        iterations++;

        // Walk basic blocks BACKWARD to converge significantly faster
        for (auto blockIt = blocks.rbegin(); blockIt != blocks.rend(); ++blockIt)
        {
            MirBlock *block = (*blockIt);
            size_t blockId = block->getId();

            auto &liveIn = m_result.m_liveIn[blockId];
            auto &liveOut = m_result.m_liveOut[blockId];
            const auto &defs = m_result.m_def[blockId];
            const auto &uses = m_result.m_use[blockId];

            // Equation 1: LiveOut[B] = Union of LiveIn[S] for all Successors S
            std::pmr::unordered_set<size_t> newLiveOut(m_arena);
            if (cfg.m_successors.contains(block->getId()))
            {
                for (size_t succ : cfg.m_successors.at(block->getId()))
                {
                    const auto &succLiveIn = m_result.m_liveIn.at(succ);
                    newLiveOut.insert(succLiveIn.begin(), succLiveIn.end());
                }
            }

            if (newLiveOut != liveOut)
            {
                liveOut = std::move(newLiveOut);
                changed = true;
            }

            // Equation 2: LiveIn[B] = Use[B] Union (LiveOut[B] Except Def[B])
            std::pmr::unordered_set<size_t> newLiveIn(uses.begin(), uses.end());
            for (size_t reg : liveOut)
            {
                if (!defs.contains(reg))
                {
                    newLiveIn.insert(reg);
                }
            }

            if (newLiveIn != liveIn)
            {
                liveIn = std::move(newLiveIn);
                changed = true;
            }
        }
    }
}

void LivenessAnalysis::computeLocalLiveness(MirFunction *func, const std::shared_ptr<DiagnosticCollector> &collector)
{
    collector->builder(DiagnosticMessageType::Diag_Debug, getName())
            << "Analyzing block-local variable generation rules (USE / DEF calculation)...";

    m_result.m_liveIn.clear();
    m_result.m_liveOut.clear();
    m_result.m_def.clear();
    m_result.m_use.clear();

    for (auto &block : func->getBlocks())
    {
        // Allocate PMR sets bound directly to our high-speed compilation arena
        size_t blockId = block->getId();
        m_result.m_def[blockId] = std::pmr::unordered_set<size_t>(m_arena);
        m_result.m_use[blockId] = std::pmr::unordered_set<size_t>(m_arena);
        m_result.m_liveIn[blockId] = std::pmr::unordered_set<size_t>(m_arena);
        m_result.m_liveOut[blockId] = std::pmr::unordered_set<size_t>(m_arena);

        auto &defs = m_result.m_def[blockId];
        auto &uses = m_result.m_use[blockId];

        // Process block variables from front to back to isolate local definitions vs first uses
        for (const auto &instr : block->getInstructions())
        {
            std::pmr::unordered_set<size_t> localDefs(m_arena);
            std::pmr::unordered_set<size_t> localUses(m_arena);

            extractRegistersFromInstruction(instr, localDefs, localUses);

            // An operand is a local 'use' if it's read before being overwritten in the block
            for (size_t reg : localUses)
            {
                if (!defs.contains(reg))
                {
                    uses.insert(reg);
                }
            }

            // An operand is a local 'def' if it's written to before being read in the block
            for (size_t reg : localDefs)
            {
                defs.insert(reg);
            }
        }

        auto log = collector->builder(DiagnosticMessageType::Diag_Trace, getName());
        log << std::pmr::string(std::format("    Block ID {:2}: Locally Def'd={}, Locally Used={}",
                                            block->getId(),
                                            defs.size(),
                                            uses.size()));
    }
}

void LivenessAnalysis::extractRegistersFromInstruction(MirInstruction *instr,
                                                       std::pmr::unordered_set<size_t> &defs,
                                                       std::pmr::unordered_set<size_t> &uses)
{
    // Loop over instruction operands and sort them into definitions or uses
    // based on instruction metadata or flags.
    for (size_t i = 0; i < instr->getOperands().size(); ++i)
    {
        MirOperand *op = instr->getOperands()[i];
        if (!op)
            continue;

        if (MirRegister *reg = op->get<MirRegister>())
        {
            size_t regId = reg->getRegId();
            if (instr->getMetadata().m_operandConstraints[i].flags & OperandFlag::Write)
            {
                defs.insert(regId);
            }
            else
            {
                uses.insert(regId);
            }
        }
    }
}
