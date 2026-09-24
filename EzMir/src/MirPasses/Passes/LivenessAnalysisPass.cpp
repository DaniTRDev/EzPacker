#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "Printer/MirPrinter.h"

#include <vector>

/**
 * Formats a block-ID -> register-set map as indented text, listing "empty" for blocks with no
 * registers. Used only by diagnostic output.
 */
std::string printMirRegMap(MirBuilderContext *ctx,
                           const std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> &map)
{
    std::string res;
    for (const auto &[blockId, defs] : map)
    {
        res += MirPrinter::printToString(ctx->getBlockById(blockId), MirPrinterDetail::General);

        if (defs.empty())
        {
            res += "\tempty\n";
        }
        else
        {
            for (const auto &def : defs)
            {
                res += "\t" + MirPrinter::printToString(def) + "\n";
            }
        }
    }

    return res;
}

/**
 * Initializes the liveness pass and its result storage using the context's global arena.
 */
LivenessAnalysisPass::LivenessAnalysisPass(MirBuilderContext *ctx) :
    m_result(ctx->getGlobalAllocator()), m_ctx(ctx), m_arena(ctx->getGlobalAllocator())
{
}

/**
 * Returns the pass identifier.
 */
const char *LivenessAnalysisPass::getName() const { return "LivenessAnalysisPass"; }

/**
 * Returns the computed liveness sets.
 */
LivenessResult *LivenessAnalysisPass::getResult() { return &m_result; }

/**
 * Runs once per function.
 */
MirPassIterationPlace LivenessAnalysisPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

/**
 * Clears all def/use/live-in/live-out sets. Invoked by the manager once before it iterates the
 * function list; results for every visited function are accumulated afterwards, keyed by the
 * globally unique block ID, exactly like CodeFlowAnalysisPass.
 */
void LivenessAnalysisPass::reset()
{
    m_result.m_def.clear();
    m_result.m_use.clear();
    m_result.m_liveIn.clear();
    m_result.m_liveOut.clear();
}

/**
 * Obtains (or computes) the CFG analysis and runs the global liveness solver over the target
 * function. Results are accumulated (not reset) across functions so the cached analysis holds
 * valid live sets for every function; the manager's single reset() sets the contract.
 */
MirPassResult LivenessAnalysisPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                        MirPassManager *passManager)
{
    MirFunction *func = *it;

    auto *cfgPass = passManager->getAnalysis<CodeFlowAnalysisPass>(m_ctx);
    auto *cfg = cfgPass->getResult();

    computeGlobalLiveness(func, cfg);

    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
}

/**
 * Solves the backward liveness dataflow equations: assigns dense indices to blocks, computes
 * block-local def/use (upward-exposed uses), builds a global register universe, runs a fixed-point
 * iteration over the CFG using DenseBitSets, then materializes the resulting live-in/live-out sets.
 */
void LivenessAnalysisPass::computeGlobalLiveness(MirFunction *func, CodeFlowResult *cfg)
{
    const auto &blocks = func->getBlocks();
    const size_t numBlocks = blocks.size();
    if (numBlocks == 0)
        return;

    // -------------------------------------------------------------------------
    // 1. Assign Contiguous [0, numBlocks) Indices to Basic Blocks
    // -------------------------------------------------------------------------
    std::pmr::vector<const MirBlock *> blockList(m_arena);
    blockList.reserve(numBlocks);

    std::pmr::unordered_map<MirId, uint32_t> blockIdToDenseIdx(numBlocks * 2, m_arena);

    uint32_t bIdx = 0;
    for (const MirBlock *block : blocks)
    {
        blockList.push_back(block);
        blockIdToDenseIdx.emplace(block->getId(), bIdx++);

        MirId bId = block->getId();
        m_result.m_def.emplace(bId, std::pmr::unordered_set<MirRegisterRef>(m_arena));
        m_result.m_use.emplace(bId, std::pmr::unordered_set<MirRegisterRef>(m_arena));
        m_result.m_liveIn.emplace(bId, std::pmr::unordered_set<MirRegisterRef>(m_arena));
        m_result.m_liveOut.emplace(bId, std::pmr::unordered_set<MirRegisterRef>(m_arena));
    }

    const auto *regInfo = func->getRegisterInfo();

    // -------------------------------------------------------------------------
    // 2. Compute Exact Block-Local Def / Use Sets (Upward Exposed Uses)
    // -------------------------------------------------------------------------
    for (size_t i = 0; i < numBlocks; ++i)
    {
        const MirBlock *block = blockList[i];
        MirId blockId = block->getId();
        auto &defSet = m_result.m_def[blockId];
        auto &useSet = m_result.m_use[blockId];

        std::pmr::vector<MirRegisterRef> readRegs(m_arena);
        std::pmr::vector<MirRegisterRef> writtenRegs(m_arena);

        for (const MirInstruction *instr : block->getInstructions())
        {
            // Upward-exposed use: register read before defined in this block
            instr->getUsedRegisters(readRegs);
            for (const auto &reg : readRegs)
            {
                if (!defSet.contains(reg))
                    useSet.insert(reg);
            }

            // Defs kill upward uses for subsequent instructions
            instr->getDefinedRegisters(writtenRegs);
            for (const auto &reg : writtenRegs)
            {
                defSet.insert(reg);
            }
        }
    }

    // -------------------------------------------------------------------------
    // 3. Build Global Register Universe
    // -------------------------------------------------------------------------
    std::pmr::vector<MirRegisterRef> globalRegUniverse(m_arena);
    std::pmr::unordered_map<MirRegisterRef, uint32_t> globalRegToIdx(m_arena);

    auto getOrAddGlobalReg = [&](const MirRegisterRef &reg) -> uint32_t
    {
        auto [it, inserted] = globalRegToIdx.emplace(reg, static_cast<uint32_t>(globalRegUniverse.size()));
        if (inserted)
            globalRegUniverse.push_back(reg);
        return it->second;
    };

    // Upward-exposed uses are live-in across block boundaries
    for (size_t i = 0; i < numBlocks; ++i)
    {
        MirId blockId = blockList[i]->getId();
        for (const auto &reg : m_result.m_use[blockId])
        {
            getOrAddGlobalReg(reg);
        }
    }

    // Registers defined in one block and used in another
    if (regInfo)
    {
        for (size_t i = 0; i < numBlocks; ++i)
        {
            MirId blockId = blockList[i]->getId();
            for (const auto &reg : m_result.m_def[blockId])
            {
                if (!reg.isVirtual())
                {
                    getOrAddGlobalReg(reg);
                    continue;
                }

                auto usesOpt = regInfo->getUses(reg.getId());
                if (usesOpt && !usesOpt->empty())
                {
                    for (const auto &use : *usesOpt)
                    {
                        if (use.m_userInst && use.m_userInst->getOwner() != blockList[i])
                        {
                            getOrAddGlobalReg(reg);
                            break;
                        }
                    }
                }
            }
        }
    }

    const size_t numGlobalBits = globalRegUniverse.size();
    if (numGlobalBits == 0)
        return;

    // -------------------------------------------------------------------------
    // 4. Populate Compact Global Def/Use BitSets
    // -------------------------------------------------------------------------
    std::pmr::vector<DenseBitSet> globalDefBits(m_arena);
    std::pmr::vector<DenseBitSet> globalUseBits(m_arena);
    std::pmr::vector<DenseBitSet> globalLiveInBits(m_arena);
    std::pmr::vector<DenseBitSet> globalLiveOutBits(m_arena);

    globalDefBits.reserve(numBlocks);
    globalUseBits.reserve(numBlocks);
    globalLiveInBits.reserve(numBlocks);
    globalLiveOutBits.reserve(numBlocks);

    for (size_t i = 0; i < numBlocks; ++i)
    {
        globalDefBits.emplace_back(numGlobalBits);
        globalUseBits.emplace_back(numGlobalBits);
        globalLiveInBits.emplace_back(numGlobalBits);
        globalLiveOutBits.emplace_back(numGlobalBits);

        MirId blockId = blockList[i]->getId();

        for (const auto &reg : m_result.m_def[blockId])
        {
            auto it = globalRegToIdx.find(reg);
            if (it != globalRegToIdx.end())
                globalDefBits[i].set(it->second);
        }

        for (const auto &reg : m_result.m_use[blockId])
        {
            auto it = globalRegToIdx.find(reg);
            if (it != globalRegToIdx.end())
                globalUseBits[i].set(it->second);
        }
    }

    // -------------------------------------------------------------------------
    // 5. Flatten CFG Successor Indices
    // -------------------------------------------------------------------------
    std::pmr::vector<std::pmr::vector<uint32_t>> denseSuccessors(numBlocks, m_arena);
    for (size_t i = 0; i < numBlocks; ++i)
    {
        MirId blockId = blockList[i]->getId();

        const std::span<const MirId> successors = cfg->getSuccessors(blockId);
        denseSuccessors[i].reserve(successors.size());
        for (MirId succId : successors)
        {
            auto succIt = blockIdToDenseIdx.find(succId);
            if (succIt != blockIdToDenseIdx.end())
                denseSuccessors[i].push_back(succIt->second);
        }
    }

    // -------------------------------------------------------------------------
    // 6. Fixed-Point Solver Loop (Backwards CFG Iteration)
    // -------------------------------------------------------------------------
    bool changed = true;
    while (changed)
    {
        changed = false;

        for (int64_t i = static_cast<int64_t>(numBlocks) - 1; i >= 0; --i)
        {
            auto &liveOut = globalLiveOutBits[i];
            auto &liveIn = globalLiveInBits[i];

            // LiveOut[B] = Union of LiveIn[S] for all Successors S
            for (uint32_t succIdx : denseSuccessors[i])
            {
                if (liveOut.unionWith(globalLiveInBits[succIdx]))
                    changed = true;
            }

            // LiveIn[B] = Use[B] | (LiveOut[B] & ~Def[B])
            if (liveIn.computeLiveIn(globalUseBits[i], liveOut, globalDefBits[i]))
                changed = true;
        }
    }

    // -------------------------------------------------------------------------
    // 7. Materialize Global LiveIn / LiveOut Sets
    // -------------------------------------------------------------------------
    for (size_t i = 0; i < numBlocks; ++i)
    {
        MirId blockId = blockList[i]->getId();
        auto &inSet = m_result.m_liveIn[blockId];
        auto &outSet = m_result.m_liveOut[blockId];

        const auto &inBits = globalLiveInBits[i];
        const auto &outBits = globalLiveOutBits[i];

        for (size_t bit = 0; bit < numGlobalBits; ++bit)
        {
            if (inBits.test(bit))
                inSet.insert(globalRegUniverse[bit]);
            if (outBits.test(bit))
                outSet.insert(globalRegUniverse[bit]);
        }
    }
}

/**
 * Traces the final def, use, live-in and live-out sets; skips all work when trace diagnostics are
 * disabled.
 */
void LivenessAnalysisPass::printResult()
{
    auto *diag = m_ctx->getDiagCollector();
    if (!diag->isDiagEnabledForType(DiagnosticMessageType::Diag_Trace))
        return;

    const auto *res = getResult();
    auto log = diag->trace(getName(), "Final Liveness Analysis");
    log.appendNote("Def\n{}", printMirRegMap(m_ctx, res->m_def));
    log.appendNote("Use\n{}", printMirRegMap(m_ctx, res->m_use));
    log.appendNote("LiveIn\n{}", printMirRegMap(m_ctx, res->m_liveIn));
    log.appendNote("LiveOut\n{}", printMirRegMap(m_ctx, res->m_liveOut));
}