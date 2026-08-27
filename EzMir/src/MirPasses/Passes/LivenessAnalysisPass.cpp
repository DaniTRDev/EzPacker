#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "MirPasses/MirPassManager.h"
#include "Printer/MirPrinter.h"

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

LivenessAnalysisPass::LivenessAnalysisPass(MirBuilderContext *ctx) :
    m_result(ctx->getGlobalAllocator()), m_ctx(ctx), m_arena(ctx->getGlobalAllocator())
{
}

const char *LivenessAnalysisPass::getName() const { return "LivenessAnalysisPass"; }

LivenessResult *LivenessAnalysisPass::getResult() { return &m_result; }

MirPassIterationPlace LivenessAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult LivenessAnalysisPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                        class MirPassManager *passManager)
{
    const MirFunction *func = *it;
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

void LivenessAnalysisPass::computeGlobalLiveness(const MirFunction *func, CodeFlowResult *cfg)
{
    m_ctx->getDiagCollector()->trace(getName(),
                                     "Analyzing global variable generation rules (live IN / OUT calculation)...");

    const auto &blocks = func->getBlocks();

    // 1. Build a dense mapping for all unique registers referenced in this function
    std::vector<MirRegisterRef> regUniverse;
    std::unordered_map<MirRegisterRef, size_t> regToIdx;

    for (auto block : blocks)
    {
        size_t blockId = block->getId();
        for (const auto &reg : m_result.m_def[blockId])
        {
            if (regToIdx.emplace(reg, regUniverse.size()).second)
                regUniverse.push_back(reg);
        }
        for (const auto &reg : m_result.m_use[blockId])
        {
            if (regToIdx.emplace(reg, regUniverse.size()).second)
                regUniverse.push_back(reg);
        }
    }

    if (regUniverse.empty())
        return;

    size_t numBits = regUniverse.size();

    // 2. Pre-allocate bitsets for all blocks (0 allocations during fixed-point loop)
    std::unordered_map<size_t, DenseBitSet> defBits;
    std::unordered_map<size_t, DenseBitSet> useBits;
    std::unordered_map<size_t, DenseBitSet> liveInBits;
    std::unordered_map<size_t, DenseBitSet> liveOutBits;

    for (auto block : blocks)
    {
        size_t blockId = block->getId();
        defBits.emplace(blockId, DenseBitSet(numBits));
        useBits.emplace(blockId, DenseBitSet(numBits));
        liveInBits.emplace(blockId, DenseBitSet(numBits));
        liveOutBits.emplace(blockId, DenseBitSet(numBits));

        for (const auto &reg : m_result.m_def[blockId])
            defBits[blockId].set(regToIdx[reg]);

        for (const auto &reg : m_result.m_use[blockId])
            useBits[blockId].set(regToIdx[reg]);
    }

    bool changed = true;
    size_t iterations = 0;

    // 3. Fixed-point solver loop using word-level bitwise operations
    while (changed)
    {
        changed = false;
        iterations++;

        // Walk basic blocks BACKWARD to converge faster
        for (auto blockIt = blocks.rbegin(); blockIt != blocks.rend(); ++blockIt)
        {
            MirBlock *block = *blockIt;
            size_t blockId = block->getId();

            auto &liveOut = liveOutBits[blockId];
            auto &liveIn = liveInBits[blockId];

            // Equation 1: LiveOut[B] = Union of LiveIn[S] for all Successors S
            if (cfg->m_successors.contains(blockId))
            {
                for (size_t succ : cfg->m_successors.at(blockId))
                {
                    if (liveInBits.contains(succ))
                    {
                        if (liveOut.unionWith(liveInBits.at(succ)))
                        {
                            changed = true;
                        }
                    }
                }
            }

            // Equation 2: LiveIn[B] = Use[B] Union (LiveOut[B] Except Def[B])
            if (liveIn.computeLiveIn(useBits[blockId], liveOut, defBits[blockId]))
            {
                changed = true;
            }
        }
    }

    // 4. Materialize final bitsets into m_result once
    for (auto block : blocks)
    {
        size_t blockId = block->getId();
        auto &liveInSet = m_result.m_liveIn[blockId];
        auto &liveOutSet = m_result.m_liveOut[blockId];

        liveInSet.clear();
        liveOutSet.clear();

        const auto &inBits = liveInBits[blockId];
        const auto &outBits = liveOutBits[blockId];

        for (size_t bit = 0; bit < regUniverse.size(); ++bit)
        {
            if (inBits.test(bit))
                liveInSet.insert(regUniverse[bit]);
            if (outBits.test(bit))
                liveOutSet.insert(regUniverse[bit]);
        }
    }
}

void LivenessAnalysisPass::computeLocalLiveness(const MirFunction *func)
{
    m_ctx->getDiagCollector()->trace(getName(),
                                     "Analyzing block-local variable generation rules (USE / DEF calculation)...");

    for (const MirBlock *block : func->getBlocks())
    {
        size_t blockId = block->getId();
        m_result.m_def[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_use[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_liveIn[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);
        m_result.m_liveOut[blockId] = std::pmr::unordered_set<MirRegisterRef>(m_arena);

        auto &defs = m_result.m_def[blockId];
        auto &uses = m_result.m_use[blockId];

        for (const MirInstruction *instr : block->getInstructions())
        {
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