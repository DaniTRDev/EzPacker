#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "MirPasses/MirPassManager.h"
#include "Printer/MirPrinter.h"

namespace
{
std::string printMirRegMap(MirBuilderContext *ctx,
                           const std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> &map)
{
    std::string res;
    for (auto &[blockId, defs] : map)
    {
        res += MirPrinter::printToString(ctx->getBlockById(blockId), MirPrinterDetail::General);

        if (defs.empty())
        {
            res += "\tempty\n";
        }
        else
        {
            for (auto &def : defs)
            {
                res += "\t" + MirPrinter::printToString(def) + "\n";
            }
        }
    }

    return res;
}
} // anonymous namespace

LivenessAnalysisPass::LivenessAnalysisPass(MirBuilderContext *ctx) :
    m_result(ctx->getGlobalAllocator()), m_ctx(ctx), m_arena(ctx->getGlobalAllocator())
{
}

const char *LivenessAnalysisPass::getName() const { return "LivenessAnalysisPass"; }

LivenessResult *LivenessAnalysisPass::getResult() { return &m_result; }

MirPassIterationPlace LivenessAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult LivenessAnalysisPass::run(std::pmr::list<MirFunction *> &funcList,
                                        std::pmr::list<MirFunction *>::iterator it,
                                        class MirPassManager *passManager)
{
    MirFunction *func = *it;
    auto diag = passManager->getDiagCollector();
    diag->trace(getName(), "Analyzing register liveness spans for function: '{}'", func->getName());

    // Recover the pre-computed Control Flow Graph directly from the Pass Manager cache
    auto cfg = passManager->getAnalysis<CodeFlowAnalysisPass>(m_ctx)->getResult();

    // Initialize and extract block-local Gen (Use) and Kill (Def) sets
    computeLocalLiveness(func);

    // Solve global fixed-point backward equations across our CFG topology paths
    computeGlobalLiveness(func, cfg);

    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

void LivenessAnalysisPass::printResult()
{
    auto diag = m_ctx->getDiagCollector();

    // Guard against running expensive map stringification if trace logging is off
    if (!diag->isDiagEnabledForType(DiagnosticMessageType::Diag_Trace))
        return;

    const auto res = getResult();

    auto log = diag->trace(getName(), "Final Liveness Analysis");
    log.appendNote("Def\n{}", printMirRegMap(m_ctx, res->m_def));
    log.appendNote("Use\n{}", printMirRegMap(m_ctx, res->m_use));
    log.appendNote("LiveIn\n{}", printMirRegMap(m_ctx, res->m_liveIn));
    log.appendNote("LiveOut\n{}", printMirRegMap(m_ctx, res->m_liveOut));
}

void LivenessAnalysisPass::reset()
{
    m_result.m_def.clear();
    m_result.m_use.clear();
    m_result.m_liveIn.clear();
    m_result.m_liveOut.clear();
}

void LivenessAnalysisPass::computeGlobalLiveness(MirFunction *func, CodeFlowResult *cfg)
{
    m_ctx->getDiagCollector()->trace(getName(),
                                     "Analyzing global variable generation rules (live IN / OUT calculation)...");

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
            std::pmr::unordered_set<MirRegisterRef> newLiveOut(m_arena);
            if (cfg->m_successors.contains(block->getId()))
            {
                for (size_t succ : cfg->m_successors.at(block->getId()))
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
            std::pmr::unordered_set<MirRegisterRef> newLiveIn(uses.begin(), uses.end());
            for (const auto &regRef : liveOut)
            {
                if (!defs.contains(regRef))
                {
                    newLiveIn.insert(regRef);
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

void LivenessAnalysisPass::computeLocalLiveness(MirFunction *func)
{
    m_ctx->getDiagCollector()->trace(getName(),
                                     "Analyzing block-local variable generation rules (USE / DEF calculation)...");

    for (auto &block : func->getBlocks())
    {
        size_t blockId = block->getId();
        m_result.m_def[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_use[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_liveIn[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_liveOut[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);

        auto &defs = m_result.m_def[blockId];
        auto &uses = m_result.m_use[blockId];

        for (const auto &instr : block->getInstructions())
        {
            // Note: getDefinedRegisters() and getUsedRegisters() return std::pmr::vector<MirRegisterRef>
            const auto &localDefs = instr->getDefinedRegisters();
            const auto &localUses = instr->getUsedRegisters();

            // Any register used before being defined in this block belongs in 'uses' (Gen)
            for (const auto &regRef : localUses)
            {
                if (!defs.contains(regRef))
                {
                    uses.insert(regRef);
                }
            }

            // Any register defined in this block belongs in 'defs' (Kill)
            for (const auto &regRef : localDefs)
            {
                defs.insert(regRef);
            }
        }
    }
}