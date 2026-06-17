#include "MirPasses/Passes/LivenessAnalysis.h"

std::string printMirRegMap(MirBuilderContext *ctx,
                           const std::pmr::unordered_map<size_t, std::pmr::unordered_set<size_t>> &map)
{
    std::string res;
    for (auto &[blockId, defs] : map)
    {
        res += MirPrinter::printToString(ctx->getBlockById(blockId), MirPrinterDetail::General);

        if (defs.empty())
        {
            res += "   empty\n";
        }
        else
        {
            for (auto &def : defs)
            {
                res += "   " + MirPrinter::printToString(ctx->getRegisterById(def)) + "\n";
            }
        }
    }

    return res;
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
    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

void LivenessAnalysis::printResult() const
{
    const auto &res = getResult();
    auto diag = m_ctx->getDiagCollector();

    auto log = diag->builder(DiagnosticMessageType::Diag_Debug, getName());
    log << "Final Liveness Analysis Matrix";

    log.appendNote(std::string("Def\n").append(printMirRegMap(m_ctx, res.m_def)).c_str(), nullptr);
    log.appendNote(std::string("Use\n").append(printMirRegMap(m_ctx, res.m_use)).c_str(), nullptr);
    log.appendNote(std::string("LiveIn\n").append(printMirRegMap(m_ctx, res.m_liveIn)).c_str(), nullptr);
    log.appendNote(std::string("LiveOut\n").append(printMirRegMap(m_ctx, res.m_liveOut)).c_str(), nullptr);
}

void LivenessAnalysis::computeGlobalLiveness(MirFunction *func, const ControlFlowResult &cfg)
{
    m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, getName())
            << "Analyzing global variable generation rules (live IN / OUT calculation)...";

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
    }
}

void LivenessAnalysis::extractRegistersFromInstruction(MirInstruction *instr,
                                                       std::pmr::unordered_set<size_t> &defs,
                                                       std::pmr::unordered_set<size_t> &uses)
{
    // Retrieve the static metadata constraints for this specific opcode
    const MirInstructionMetadata &meta = getMeta(instr->getOpCode());

    for (size_t i = 0; i < instr->getOperands().size(); ++i)
    {
        // Guard against matching more operands than we have metadata constraints for
        if (i >= meta.m_operandConstraints.size())
            break;

        MirOperand *op = instr->getOperands()[i];
        if (MirRegister *reg = op->get<MirRegister>())
        {
            size_t regId = reg->getRegId();
            OperandFlag flags = meta.m_operandConstraints[i].flags;

            // Check if the metadata says this operand position writes
            if (flags & OperandFlag::Write)
            {
                defs.insert(regId);
            }

            // Check if the metadata says this operand position reads
            // (Note: ReadWrite flags will correctly hit BOTH blocks)
            if (flags & OperandFlag::Read)
            {
                uses.insert(regId);
            }
        }
    }
}
