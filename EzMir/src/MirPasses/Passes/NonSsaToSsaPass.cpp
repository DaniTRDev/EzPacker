#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Function/MirFunction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/NonSsaToSsaPass.h"
#include "Operand/MirOperands.h"
#include "Operand/MirOperandBuilder.h"
#include <algorithm>

/**
 * Initializes the SSA construction pass and its intermediate data using the context's global arena.
 */
NonSsaToSsaPass::NonSsaToSsaPass(class MirBuilderContext *ctx) :
    m_ctx(ctx), m_result(ctx->getGlobalAllocator()), m_resc(ctx->getGlobalAllocator())
{
}

/**
 * Returns the pass identifier.
 */
const char *NonSsaToSsaPass::getName() const { return "NonSsaToSsaPass"; }

/**
 * Runs once per function.
 */
MirPassIterationPlace NonSsaToSsaPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

/**
 * Runs the full SSA construction pipeline on the target function: collect def sites, compute
 * post-order/dominators, build dominance frontiers, insert phi nodes and rename variables. Always
 * reports that the MIR was modified.
 */
MirPassResult NonSsaToSsaPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it, MirPassManager *passManager)
{
    MirFunction *func = *it;

    // If the function already contains PHI nodes, it is already in SSA form.
    // Running SSA reconstruction would destroy existing PHI operand mappings.
    for (const MirBlock *block : func->getBlocks())
    {
        for (const MirInstruction *inst : block->getInstructions())
        {
            if (inst->hasOpcode(MirInstructionOpCode::PHI))
            {
                m_ctx->getDiagCollector()->trace("NonSsaToSsaPass",
                                                 "Function '{}' already contains PHI nodes; skipping SSA construction",
                                                 func->getName());
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
            }
        }
    }

    CodeFlowResult *cfg = passManager->getAnalysis<CodeFlowAnalysisPass>(m_ctx)->getResult();

    m_ctx->getDiagCollector()->trace("NonSsaToSsaPass", "--- Starting SSA Construction for function ---");

    buildVirtualRegDefPlaces(func);
    buildPostOrderIndexList(cfg, func, passManager);
    buildImmDomTree(cfg, func, passManager);
    buildDominanceFrontier(cfg, func, passManager);
    insertPhiNodes(cfg, func);
    renameVariables(cfg, func);

    m_ctx->getDiagCollector()->trace("NonSsaToSsaPass", "--- Completed SSA Construction ---");

    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

/**
 * Returns the intermediate dominator/frontier data collected during the last run.
 */
NonSsaToSsaPassResult *NonSsaToSsaPass::getResult() { return &m_result; }

/**
 * No-op; this pass does not emit a dedicated result report.
 */
void NonSsaToSsaPass::printResult() {}

/**
 * No-op; per-function data is rebuilt from scratch on each run.
 */
void NonSsaToSsaPass::reset() {}

/**
 * Creates a PHI instruction for regId with numPredecessors identical incoming operands, which the
 * rename phase later replaces with the reaching definitions.
 */
MirInstruction *
NonSsaToSsaPass::createPhiInstruction(MirInstructionBuilder *iBuilder, MirId regId, size_t numPredecessors)
{
    MirRegister *reg = m_ctx->getRegisterById(regId);
    MirInstruction *phi = iBuilder->PHI(reg);

    for (size_t i = 0; i < numPredecessors; ++i)
    {
        iBuilder->addOperand(phi, reg);
    }

    return phi;
}

/**
 * Scans every block/instruction to record, per virtual register, the set of blocks that define it.
 */
void NonSsaToSsaPass::buildVirtualRegDefPlaces(MirFunction *func)
{
    auto &defSites = m_result.m_defSites;

    if (func->getEntryPoint())
    {
        MirId entryId = func->getEntryPoint()->getId();
        for (MirRegister *param : func->getParameters())
        {
            if (param && param->isVirtual())
            {
                defSites[param->getRegId()].insert(entryId);
            }
        }
    }

    for (const MirBlock *block : func->getBlocks())
    {
        MirId bId = block->getId();

        std::pmr::vector<MirRegisterRef> defs(m_resc);
        for (const MirInstruction *inst : block->getInstructions())
        {
            inst->getDefinedRegisters(defs);
            for (const auto &def : defs)
            {
                if (def.isVirtual())
                {
                    defSites[def.getId()].insert(bId);

                    m_ctx->getDiagCollector()->trace("NonSsaToSsaPass",
                                                     "Tracked definition of virtual register {} in block {}",
                                                     def.getId(),
                                                     bId)
                            << inst->getSourceRef();
                }
            }
        }
    }
}

/**
 * Computes dominance frontiers using the classic "runner walks up the dominator tree" method: for
 * each join block, each predecessor is advanced toward the block's immediate dominator, adding the
 * block to every node along the way.
 */
void NonSsaToSsaPass::buildDominanceFrontier(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager)
{
    auto &domFrontier = m_result.m_domFrontier;
    auto &idom = m_result.m_immDomTree;

    for (MirBlock *b : func->getBlocks())
    {
        MirId bId = b->getId();

        auto idomIt = idom.find(bId);
        if (idomIt == idom.end())
            continue;

        MirId bIdom = idomIt->second;
        const std::span<const MirId> predecessors = cfg->getPredecessors(bId);

        if (predecessors.size() < 2)
            continue;

        for (MirId p : predecessors)
        {
            MirId runner = p;

            while (runner != bIdom)
            {
                auto runnerIdomIt = idom.find(runner);
                if (runnerIdomIt == idom.end())
                    break;

                domFrontier[runner].insert(bId);

                m_ctx->getDiagCollector()->trace("NonSsaToSsaPass",
                                                 "Added block {} to Dominance Frontier of block {}",
                                                 bId,
                                                 runner);

                MirId nextRunner = runnerIdomIt->second;
                if (nextRunner == runner)
                    break;

                runner = nextRunner;
            }
        }
    }
}

/**
 * Computes the immediate dominator tree using the Cooper-Harvey-Kennedy iterative algorithm: the
 * entry dominates itself and each block's idom is the intersection of its processed predecessors,
 * iterated to a fixed point.
 */
void NonSsaToSsaPass::buildImmDomTree(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager)
{
    auto &domTree = m_result.m_immDomTree;
    auto &postOrderIndexes = m_result.m_postOrderIndexes;
    auto &postOrderNodes = m_result.m_postOrderNodes;
    bool changed = true;

    MirId entryId = func->getEntryPoint()->getId();
    domTree[entryId] = entryId;

    auto intersect = [&](MirId b1, MirId b2) -> MirId
    {
        MirId finger1 = b1;
        MirId finger2 = b2;

        while (finger1 != finger2)
        {
            // Use find() rather than operator[] so a missing index never silently inserts a zero
            // entry, which could spin the loop forever on a 0 <-> 0 comparison.
            auto idx1It = postOrderIndexes.find(finger1);
            auto idx2It = postOrderIndexes.find(finger2);
            if (idx1It == postOrderIndexes.end() || idx2It == postOrderIndexes.end())
                break;

            MirId &finger = (idx1It->second < idx2It->second) ? finger1 : finger2;
            auto next = domTree.find(finger);
            if (next == domTree.end() || next->second == finger)
                break;

            finger = next->second;
        }
        return finger1;
    };

    while (changed)
    {
        changed = false;

        for (auto it = postOrderNodes.rbegin(); it != postOrderNodes.rend(); ++it)
        {
            MirId blockId = *it;

            if (blockId == entryId)
            {
                continue;
            }

            MirId newIdom = 0;
            bool foundFirst = false;

            const std::span<const MirId> predecessors = cfg->getPredecessors(blockId);
            for (MirId pred : predecessors)
            {
                if (domTree.find(pred) != domTree.end())
                {
                    if (!foundFirst)
                    {
                        newIdom = pred;
                        foundFirst = true;
                    }
                    else
                    {
                        newIdom = intersect(pred, newIdom);
                    }
                }
            }

            if (foundFirst && domTree[blockId] != newIdom)
            {
                domTree[blockId] = newIdom;
                changed = true;

                m_ctx->getDiagCollector()->trace("NonSsaToSsaPass",
                                                 "Updated Immediate Dominator for block {} -> {}",
                                                 blockId,
                                                 newIdom);
            }
        }
    }
}

/**
 * Produces a depth-first post-order of reachable blocks starting from the entry point and assigns
 * each block an index used by the dominator intersection routine.
 */
void NonSsaToSsaPass::buildPostOrderIndexList(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager)
{
    MirId entryId = func->getEntryPoint()->getId();

    auto &postOrder = m_result.m_postOrderNodes;
    postOrder.reserve(func->getBlockCount());

    std::pmr::unordered_set<MirId> visited(m_resc);

    auto dfs = [&](auto &self, MirId currentBlock) -> void
    {
        visited.insert(currentBlock);

        for (MirId succ : cfg->getSuccessors(currentBlock))
        {
            if (visited.find(succ) == visited.end())
            {
                self(self, succ);
            }
        }
        postOrder.push_back(currentBlock);
    };

    dfs(dfs, entryId);

    for (size_t i = 0; i < postOrder.size(); ++i)
    {
        m_result.m_postOrderIndexes[postOrder[i]] = i;
        m_ctx->getDiagCollector()->trace("NonSsaToSsaPass", "PostOrder [{}] = Block {}", i, postOrder[i]);
    }
}

/**
 * Places PHI nodes using the iterated dominance-frontier algorithm: for each register's definition
 * sites, walk the frontier worklist, inserting a PHI at each unvisited frontier block and
 * continuing to that block's frontier.
 */
void NonSsaToSsaPass::insertPhiNodes(CodeFlowResult *cfg, MirFunction *func)
{
    auto &domFrontier = m_result.m_domFrontier;
    auto &defSites = m_result.m_defSites;
    MirInstructionBuilder iBuilder(m_ctx, nullptr, InsertionType::InsertBefore, {});

    // Visit registers (and their def/dominance-frontier sets) in sorted order so phi placement is
    // independent of unordered container iteration order.
    std::pmr::vector<MirId> regIds(m_resc);
    regIds.reserve(defSites.size());
    for (const auto &[regId, definingBlocks] : defSites)
    {
        regIds.push_back(regId);
    }
    std::sort(regIds.begin(), regIds.end());

    for (MirId regId : regIds)
    {
        const auto &definingBlocks = defSites.at(regId);

        std::pmr::vector<MirId> worklist(m_resc);
        worklist.reserve(definingBlocks.size());

        std::pmr::unordered_set<MirId> inWorklist(m_resc);
        std::pmr::unordered_set<MirId> hasPhi(m_resc);

        std::pmr::vector<MirId> sortedDefBlocks(definingBlocks.begin(), definingBlocks.end(), m_resc);
        std::sort(sortedDefBlocks.begin(), sortedDefBlocks.end());

        for (MirId bId : sortedDefBlocks)
        {
            worklist.push_back(bId);
            inWorklist.insert(bId);
        }

        while (!worklist.empty())
        {
            MirId currentBlockId = worklist.back();
            worklist.pop_back();
            inWorklist.erase(currentBlockId);

            auto dfIt = domFrontier.find(currentBlockId);
            if (dfIt == domFrontier.end())
                continue;

            std::pmr::vector<MirId> dfBlocks(dfIt->second.begin(), dfIt->second.end(), m_resc);
            std::sort(dfBlocks.begin(), dfBlocks.end());

            for (MirId dfBlockId : dfBlocks)
            {
                if (hasPhi.find(dfBlockId) == hasPhi.end())
                {
                    hasPhi.insert(dfBlockId);

                    MirBlock *targetBlock = func->getBlock(dfBlockId);
                    if (!targetBlock)
                        continue;

                    iBuilder.setInsertionPoint(targetBlock, InsertionType::InsertBefore, targetBlock->begin());

                    auto numPredsIt = cfg->getPredecessors(dfBlockId);
                    size_t numPreds = numPredsIt.size();
                    createPhiInstruction(&iBuilder, regId, numPreds);

                    auto diag = m_ctx->getDiagCollector()->trace("NonSsaToSsaPass",
                                                                 "Inserted PHI node for base reg {} in DF block {}",
                                                                 regId,
                                                                 dfBlockId);
                    diag.appendNote("Required incoming predecessor paths: {}", numPreds);

                    if (definingBlocks.find(dfBlockId) == definingBlocks.end() &&
                        inWorklist.find(dfBlockId) == inWorklist.end())
                    {
                        worklist.push_back(dfBlockId);
                        inWorklist.insert(dfBlockId);
                    }
                }
            }
        }
    }
}

/**
 * Renames definitions and uses to SSA form during a dominator-tree walk: PHI destinations and WRITE
 * operands get fresh registers pushed onto a per-base-register stack, READ operands are replaced
 * with the current reaching definition (or an "undef" register when none exists), and PHI
 * successor slots are filled in. Stacks are rolled back after each subtree to preserve dominator
 * scoping.
 */
void NonSsaToSsaPass::renameVariables(CodeFlowResult *cfg, MirFunction *func)
{
    DiagnosticCollector *collector = m_ctx->getDiagCollector();
    MirOperandBuilder oBuilder(m_ctx);
    std::pmr::unordered_map<MirId, std::pmr::vector<MirId>> domChildren(m_resc);

    for (MirBlock *b : func->getBlocks())
    {
        MirId bId = b->getId();
        auto it = m_result.m_immDomTree.find(bId);
        if (it != m_result.m_immDomTree.end())
        {
            MirId parent = it->second;
            if (parent != bId)
            {
                domChildren[parent].push_back(bId);
            }
        }
    }

    std::pmr::unordered_map<MirId, std::pmr::vector<MirRegister *>> varStacks(m_resc);
    std::pmr::unordered_map<MirInstruction *, MirId> phiOriginalReg(m_resc);
    std::pmr::unordered_map<MirId, size_t> varCounters(m_resc);
    std::pmr::unordered_map<MirId, MirRegister *> undefRegs(m_resc);

    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            if (inst->hasOpcode(MirInstructionOpCode::PHI))
            {
                MirRegister *dst = inst->getOpAs<MirRegister>(0);
                if (dst && dst->isVirtual())
                {
                    phiOriginalReg[inst] = dst->getRegId();
                }
            }
        }
    }

    auto renameBlock = [&](auto &self, MirId blockId) -> void
    {
        MirBlock *block = func->getBlock(blockId);
        if (!block)
            return;

        collector->trace("NonSsaToSsaPass", "---> Visiting block {} for renaming", blockId);

        MirInstructionBuilder iBuilder(m_ctx, block, InsertionType::Append);
        std::pmr::vector<MirId> pushedRegisters(m_resc);

        // --- 0. Initialize function parameters at entry block ---
        if (func->getEntryPoint() && blockId == func->getEntryPoint()->getId())
        {
            for (MirRegister *param : func->getParameters())
            {
                if (param && param->isVirtual())
                {
                    MirId pId = param->getRegId();
                    varStacks[pId].push_back(param);
                    pushedRegisters.push_back(pId);
                }
            }
        }

        // --- A. Process PHI destinations ---
        for (MirInstruction *inst : block->getInstructions())
        {
            if (!inst->hasOpcode(MirInstructionOpCode::PHI))
                break;

            MirRegister *dst = inst->getOpAs<MirRegister>(0);
            if (!dst || !dst->isVirtual())
                continue;

            MirId origReg = phiOriginalReg[inst];
            size_t count = ++varCounters[origReg];

            MirRegister *newReg = oBuilder.buildVReg(dst->getMirType(),
                                                     std::format("{}.{}", dst->getName().c_str(), count).c_str(),
                                                     dst->getSourceRef(),
                                                     dst->getRegClass());

            iBuilder.swapOperand(inst, newReg, 0);

            varStacks[origReg].push_back(newReg);
            pushedRegisters.push_back(origReg);

            collector->trace("NonSsaToSsaPass",
                             "Renamed PHI destination from base reg {} -> new reg {}",
                             origReg,
                             newReg->getRegId())
                    << inst->getSourceRef();
        }

        // --- B. Process normal instructions ---
        for (MirInstruction *inst : block->getInstructions())
        {
            if (inst->hasOpcode(MirInstructionOpCode::PHI))
                continue;

            auto &ops = inst->getOperands();

            // Helper to resolve a virtual register read to its current reaching definition.
            auto renameReadReg = [&](MirRegister *op) -> MirRegister *
            {
                if (!op || !op->isVirtual())
                {
                    return op;
                }
                MirId origReg = op->getRegId();
                auto &stack = varStacks[origReg];
                if (!stack.empty())
                {
                    collector->trace("NonSsaToSsaPass",
                                     "Renamed READ operand from base reg {} -> reaching definition reg {}",
                                     origReg,
                                     stack.back()->getRegId())
                            << inst->getSourceRef();
                    return stack.back();
                }
                if (undefRegs.find(origReg) == undefRegs.end())
                {
                    undefRegs[origReg] = oBuilder.buildVReg(op->getMirType(),
                                                            "undef",
                                                            op->getSourceRef(),
                                                            op->getRegClass());
                }
                auto diag = collector->trace(
                        "NonSsaToSsaPass",
                        "Uninitialized READ detected for base reg {}. Resolved to undef reg {}",
                        origReg,
                        undefRegs[origReg]->getRegId());
                diag << inst->getSourceRef();
                diag.appendNote("Inst reads from a register that lacks a dominator definition path.");
                return undefRegs[origReg];
            };

            // 1. Rename READ operands first. Test the flag bitwise so ReadWrite operands are
            //    recognized as reads too (matching getUsedRegisters/getDefinedRegisters).
            for (size_t i = 0; i < inst->getOperandCount(); i++)
            {
                if (inst->getOperandFlag(i) & MirOperandFlag::Read)
                {
                    MirRegister *op = inst->getOpAs<MirRegister>(i);
                    if (op && op->isVirtual())
                    {
                        iBuilder.swapOperand(inst, renameReadReg(op), i);
                    }
                }
                if (auto *mem = inst->getOperand(i)->get<MirMemory>())
                {
                    if (mem->getBase() && mem->getBase()->isVirtual())
                    {
                        mem->setBase(renameReadReg(mem->getBase()));
                    }
                    if (mem->getIndex() && mem->getIndex()->isVirtual())
                    {
                        mem->setIndex(renameReadReg(mem->getIndex()));
                    }
                }
            }

            // 2. Rename WRITE operands. Pure writes get a fresh SSA name; ReadWrite operands were
            //    already renamed as reads above and are left untouched to avoid assigning a new
            //    name off the post-rename reaching definition.
            for (size_t i = 0; i < inst->getOperandCount(); ++i)
            {
                MirOperandFlag writeFlag = inst->getOperandFlag(i);
                if ((writeFlag & MirOperandFlag::Write) && !(writeFlag & MirOperandFlag::Read))
                {
                    MirRegister *op = inst->getOpAs<MirRegister>(i);
                    if (op && op->isVirtual())
                    {
                        MirId origReg = op->getRegId();
                        size_t count = ++varCounters[origReg];

                        MirRegister *newReg =
                                oBuilder.buildVReg(op->getMirType(),
                                                   std::format("{}.{}", op->getName().c_str(), count).c_str(),
                                                   op->getSourceRef(),
                                                   op->getRegClass());

                        iBuilder.swapOperand(inst, newReg, i);
                        varStacks[origReg].push_back(newReg);
                        pushedRegisters.push_back(origReg);

                        collector->trace("NonSsaToSsaPass",
                                         "Renamed WRITE operand from base reg {} -> new reg {}",
                                         origReg,
                                         newReg->getRegId())
                                << inst->getSourceRef();
                    }
                }
            }
        }

        // --- C. Populate PHI arguments in CFG Successors ---
        for (MirId succId : cfg->getSuccessors(blockId))
        {
            MirBlock *succBlock = func->getBlock(succId);
            if (!succBlock)
                continue;

            const std::span<const MirId> preds = cfg->getPredecessors(succId);
            auto predIt = std::find(preds.begin(), preds.end(), blockId);
            if (predIt == preds.end())
                continue;

            size_t predIndex = std::distance(preds.begin(), predIt);

            for (MirInstruction *inst : succBlock->getInstructions())
            {
                if (!inst->hasOpcode(MirInstructionOpCode::PHI))
                    break;

                auto it = phiOriginalReg.find(inst);
                if (it == phiOriginalReg.end())
                    continue;

                MirId origReg = it->second;
                auto &stack = varStacks[origReg];

                MirRegister *activeReg = nullptr;
                if (!stack.empty())
                {
                    activeReg = stack.back();
                }
                else
                {
                    if (undefRegs.find(origReg) == undefRegs.end())
                    {
                        MirRegister *phiDst = inst->getOpAs<MirRegister>(0);
                        undefRegs[origReg] = oBuilder.buildVReg(phiDst->getMirType(),
                                                                "undef",
                                                                phiDst->getSourceRef(),
                                                                phiDst->getRegClass());
                    }
                    activeReg = undefRegs[origReg];
                }

                auto &ops = inst->getOperands();
                if ((1 + predIndex) < ops.size())
                {
                    iBuilder.swapOperand(inst, activeReg, 1 + predIndex);
                    collector->trace("NonSsaToSsaPass",
                                     "Populated PHI incoming val in block {} (path from {}) for base reg {} -> "
                                     "resolved to reg {}",
                                     succId,
                                     blockId,
                                     origReg,
                                     activeReg->getRegId())
                            << inst->getSourceRef();
                }
            }
        }

        // --- D. Recurse down the Dominator Tree ---
        for (MirId childId : domChildren[blockId])
        {
            self(self, childId);
        }

        // --- E. Scope Rollback ---
        for (MirId origReg : pushedRegisters)
        {
            varStacks[origReg].pop_back();
        }

        collector->trace("NonSsaToSsaPass",
                         "<--- Leaving block {}, rolled back {} definitions",
                         blockId,
                         pushedRegisters.size());
    };

    renameBlock(renameBlock, func->getEntryPoint()->getId());
}