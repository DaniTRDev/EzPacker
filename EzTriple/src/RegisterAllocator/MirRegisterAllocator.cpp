#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterBank.h"
#include "Printer/MirPrinter.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
#include <bitset>

namespace
{
bool isSamePhysReg(const MirRegisterRef &a, const MirRegisterRef &b)
{
    if (!a.isPhysical() || !b.isPhysical())
        return false;
    if (a.getId() != b.getId())
        return false;
    if (!a.getClass() || !b.getClass())
        return true;
    return a.getClass()->getBank() == b.getClass()->getBank();
}
} // namespace

/**
 * Builds the interference graph from backward liveness: live-out sets seed the nodes, each
 * definition interferes with every simultaneously-live value, calls clobber caller-saved
 * registers, and the frame/stack pointers are reserved.
 */
bool MirRegisterAllocator::buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    CallingConvDesc *cc = func->getCallingConv();
    const auto analysisData = func->getAnalysisData();
    const auto &blockList = func->getBlocks();

    // Instruction stream drives spill costs, so invalidate any cost cache built for a previous state.
    ctx->m_spillCostsValid = false;

    // Reserve the frame and stack pointer registers first so dense node insertion tracks m_reservedDense.
    if (cc)
    {
        auto reserveBankReg = [&](MirRegisterRef reg)
        {
            ctx->m_reservedRegs.insert(reg);
            if (auto *cls = reg.getClass())
            {
                if (auto *bank = cls->getBank())
                {
                    for (const auto &[name, otherClass] : bank->getClasses())
                    {
                        for (const auto &[regName, regDesc] : otherClass->getRegs())
                        {
                            if (regDesc->m_id == reg.getId())
                            {
                                ctx->m_reservedRegs.insert(MirRegisterRef::preg(regDesc));
                            }
                        }
                    }
                }
            }
        };

        if (cc->hasFramePointer(func))
        {
            MirRegisterRef fpReg = cc->getFramePointerReg();
            if (fpReg.isPhysical())
            {
                reserveBankReg(fpReg);
            }
        }

        MirRegisterRef spReg = cc->getStackPointerReg();
        if (spReg.isPhysical())
        {
            reserveBankReg(spReg);
        }
    }

    DenseBitSet live(ctx->m_allocator);
    std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);
    std::pmr::vector<MirRegisterRef> uses(ctx->m_allocator);

    for (MirBlock *block : blockList)
    {
        size_t blockId = block->getId();
        const auto &liveOutSet = liveness->m_liveOut[blockId];

        live.reset();
        for (MirRegisterRef regRef : liveOutSet)
        {
            uint32_t id = addNode(regRef, ctx);
            live.resize(ctx->m_adjCapacity);
            live.set(id);
        }

        const auto &instructions = block->getInstructions();
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it)
        {
            MirInstruction *inst = *it;
            inst->getDefinedRegisters(defs);
            inst->getUsedRegisters(uses);

            if (isInstructionDAlloc(inst))
            {
                analysisData->m_hasDynamicAllocs = true;
            }

            bool isCall = (inst->getOpCode() == MirInstructionOpCode::CALL) ||
                          (inst->getTargetDesc() &&
                           (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsCall));
            if (isCall)
            {
                analysisData->m_hasCalls = true;
                if (cc)
                {
                    const auto &callerSaved = cc->getAllCallerSavedRegs();
                    for (const auto &csReg : callerSaved)
                    {
                        auto *csBank = csReg.getClass() ? csReg.getClass()->getBank() : nullptr;
                        uint32_t csId = addNode(csReg, ctx);
                        live.forEachSetBit(
                                [&](size_t liveBit)
                                {
                                    uint32_t liveId = static_cast<uint32_t>(liveBit);
                                    if (csId == liveId)
                                        return;
                                    const MirRegisterRef &liveRegRef = ctx->m_idToReg[liveId];
                                    auto *liveBank =
                                            liveRegRef.getClass() ? liveRegRef.getClass()->getBank() : nullptr;
                                    if (!csBank || !liveBank || csBank == liveBank)
                                    {
                                        ctx->addEdge(csId, liveId);
                                    }
                                });
                    }
                }
            }

            // Add nodes and interference edges for DEFs
            for (const MirRegisterRef &defRegRef : defs)
            {
                auto *defBank = defRegRef.getClass() ? defRegRef.getClass()->getBank() : nullptr;
                uint32_t defId = addNode(defRegRef, ctx);
                live.forEachSetBit(
                        [&](size_t liveBit)
                        {
                            uint32_t liveId = static_cast<uint32_t>(liveBit);
                            if (defId == liveId)
                                return;
                            const MirRegisterRef &liveRegRef = ctx->m_idToReg[liveId];
                            auto *liveBank = liveRegRef.getClass() ? liveRegRef.getClass()->getBank() : nullptr;
                            if (!defBank || !liveBank || defBank == liveBank)
                            {
                                ctx->addEdge(defId, liveId);
                            }
                        });
            }

            // In 2-address architectures (like x86-64), instructions dst = src1 OP src2 copy src1
            // into dst before the operation. If dst shares a physical register with src2 (operand index >= 2),
            // materializing the move would destroy src2 before it can be read.
            if (defs.size() == 1 && inst->getOperandCount() >= 3)
            {
                const MirRegisterRef &defReg = defs[0];
                uint32_t defId = addNode(defReg, ctx);
                for (size_t i = 2; i < inst->getOperandCount(); ++i)
                {
                    if (inst->getOperand(i) && inst->getOperand(i)->isOfType<MirRegister>())
                    {
                        MirRegisterRef srcReg = inst->getOperand(i)->get<MirRegister>()->getRef();
                        uint32_t srcId = addNode(srcReg, ctx);
                        ctx->addEdge(defId, srcId);
                    }
                }
            }

            // Erase DEFs from live set
            for (const MirRegisterRef &defRegRef : defs)
            {
                uint32_t defId = ctx->findNode(defRegRef);
                if (defId != RegisterAllocatorCtx::InvalidNode)
                {
                    live.clearBit(defId);
                }
            }

            // Add USEs to live set
            for (const MirRegisterRef &useRegRef : uses)
            {
                uint32_t useId = addNode(useRegRef, ctx);
                live.resize(ctx->m_adjCapacity);
                live.set(useId);
            }
        }
    }

    return true;
}

/**
 * Conservatively coalesces copy-related and two-address virtual/physical registers
 * using Briggs and George heuristics. Returns true if any registers were coalesced.
 */
bool MirRegisterAllocator::coalesce(RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    if (!func)
    {
        return false;
    }

    struct CoalesceCandidate
    {
        uint32_t dstId;
        uint32_t srcId;
    };

    std::pmr::vector<CoalesceCandidate> candidates(ctx->m_allocator);
    std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);

    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            bool isMove = (inst->getOpCode() == MirInstructionOpCode::MOV) ||
                          (inst->getTargetDesc() &&
                           (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsMove));

            if (isMove && inst->getOperandCount() >= 2)
            {
                MirOperand *op0 = inst->getOperand(0);
                MirOperand *op1 = inst->getOperand(1);
                if (op0 && op1 && op0->isOfType<MirRegister>() && op1->isOfType<MirRegister>())
                {
                    uint32_t dstId = ctx->findNode(op0->get<MirRegister>()->getRef());
                    uint32_t srcId = ctx->findNode(op1->get<MirRegister>()->getRef());
                    if (dstId != RegisterAllocatorCtx::InvalidNode &&
                        srcId != RegisterAllocatorCtx::InvalidNode && dstId != srcId)
                    {
                        candidates.push_back({ dstId, srcId });
                    }
                }
            }
            else if (inst->getOperandCount() >= 3)
            {
                inst->getDefinedRegisters(defs);
                if (defs.size() == 1)
                {
                    MirOperand *op0 = inst->getOperand(0);
                    MirOperand *op1 = inst->getOperand(1);
                    if (op0 && op1 && op0->isOfType<MirRegister>() && op1->isOfType<MirRegister>())
                    {
                        MirRegisterRef defRef = defs[0];
                        MirRegisterRef op0Ref = op0->get<MirRegister>()->getRef();
                        MirRegisterRef op1Ref = op1->get<MirRegister>()->getRef();
                        if (defRef == op0Ref && op0Ref != op1Ref)
                        {
                            uint32_t op0Id = ctx->findNode(op0Ref);
                            uint32_t op1Id = ctx->findNode(op1Ref);
                            if (op0Id != RegisterAllocatorCtx::InvalidNode &&
                                op1Id != RegisterAllocatorCtx::InvalidNode)
                            {
                                candidates.push_back({ op0Id, op1Id });
                            }
                        }
                    }
                }
            }
        }
    }

    if (candidates.empty())
    {
        return false;
    }

    // Ensure color limits are precomputed for all nodes before candidate scanning.
    for (uint32_t id = 0; id < ctx->m_idToReg.size(); ++id)
    {
        (void)ctx->getColorLimit(ctx->m_idToReg[id].getClass());
    }

    DenseBitSet combinedNeighbors(ctx->m_adjCapacity, ctx->m_allocator);
    std::pmr::vector<uint32_t> neighborsOfV(ctx->m_allocator);

    bool anyCoalesced = false;
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (const auto &cand : candidates)
        {
            uint32_t u = ctx->getLeaderId(cand.dstId);
            uint32_t v = ctx->getLeaderId(cand.srcId);

            if (u == v)
            {
                continue;
            }

            // Ensure physical register is always 'u' if there is one
            if (ctx->m_idToReg[u].isVirtual() && ctx->m_idToReg[v].isPhysical())
            {
                std::swap(u, v);
            }

            const MirRegisterRef uRef = ctx->m_idToReg[u];
            const MirRegisterRef vRef = ctx->m_idToReg[v];

            // Cannot coalesce two distinct physical registers
            if (uRef.isPhysical() && vRef.isPhysical())
            {
                continue;
            }

            // Cannot coalesce reserved physical registers
            if (ctx->m_reservedDense.test(u) || ctx->m_reservedDense.test(v))
            {
                continue;
            }

            // Registers must belong to the same register class
            if (uRef.getClass() != vRef.getClass())
            {
                continue;
            }

            // If they interfere, they cannot be coalesced
            if (ctx->m_adjBits[u].test(v))
            {
                continue;
            }

            bool canCoalesce = false;

            if (uRef.isPhysical())
            {
                // George's criterion: for each neighbor t of v,
                // either t already interferes with u, or t is a low-degree virtual node (deg(t) < K_t)
                bool satisfiesGeorge = true;
                for (uint32_t t : ctx->m_adjList[v])
                {
                    if (t == u || ctx->m_adjBits[u].test(t))
                    {
                        continue;
                    }

                    if (ctx->m_idToReg[t].isVirtual())
                    {
                        const size_t degT = ctx->m_adjList[t].size();
                        const size_t Kt = ctx->m_classColorLimits[ctx->m_nodeClassIdx[t]];
                        if (degT < Kt)
                        {
                            continue;
                        }
                    }

                    satisfiesGeorge = false;
                    break;
                }

                canCoalesce = satisfiesGeorge;
            }
            else
            {
                // Both are virtual: Briggs' criterion
                // Number of combined neighbors with significant degree (>= K_t) must be < K_u
                const size_t Ku = ctx->m_classColorLimits[ctx->m_nodeClassIdx[u]];

                combinedNeighbors.copyOr(ctx->m_adjBits[u], ctx->m_adjBits[v]);
                combinedNeighbors.clearBit(u);
                combinedNeighbors.clearBit(v);

                size_t significantDegreeCount = 0;
                combinedNeighbors.forEachSetBit(
                        [&](size_t bit)
                        {
                            const uint32_t t = static_cast<uint32_t>(bit);
                            if (ctx->m_idToReg[t].isPhysical())
                            {
                                significantDegreeCount++;
                            }
                            else
                            {
                                const size_t degT = ctx->m_adjList[t].size();
                                const size_t Kt = ctx->m_classColorLimits[ctx->m_nodeClassIdx[t]];
                                if (degT >= Kt)
                                {
                                    significantDegreeCount++;
                                }
                            }
                        });

                canCoalesce = (significantDegreeCount < Ku);
            }

            if (canCoalesce)
            {
                // Coalesce v into u
                ctx->m_leader[v] = u;
                ctx->m_coalescedAway.set(v);

                if (uRef.isPhysical() && ctx->m_targetFunction)
                {
                    if (CallingConvDesc *cc = ctx->m_targetFunction->getCallingConv())
                    {
                        const auto &calleeSaved = cc->getCalleeSavedRegs(uRef.getClass());
                        for (auto &reg : calleeSaved)
                        {
                            if (isSamePhysReg(reg, uRef))
                            {
                                MirFunctionBuilder fBuilder(ctx->m_ctx);
                                fBuilder.addPhysRegUse(ctx->m_targetFunction, reg);
                                break;
                            }
                        }
                    }
                }

                if (ctx->m_unspillableDense.test(v))
                {
                    ctx->m_unspillableDense.set(u);
                    ctx->m_unspillableDense.clearBit(v);
                    ctx->m_unspillableRegs.insert(uRef);
                    ctx->m_unspillableRegs.erase(vRef);
                }

                // Symmetrically merge active interference edges of v into u
                neighborsOfV.assign(ctx->m_adjList[v].begin(), ctx->m_adjList[v].end());
                for (uint32_t w : neighborsOfV)
                {
                    ctx->removeEdge(v, w);
                    if (w != u)
                    {
                        ctx->addEdge(u, w);
                    }
                }

                anyCoalesced = true;
                changed = true;
            }
            else
            {
                // Add affinity bias so selectColors prefers the same physical register
                auto &affU = ctx->m_affinity[u];
                if (std::find(affU.begin(), affU.end(), v) == affU.end())
                {
                    affU.push_back(v);
                }

                auto &affV = ctx->m_affinity[v];
                if (std::find(affV.begin(), affV.end(), u) == affV.end())
                {
                    affV.push_back(u);
                }
            }
        }
    }

    return anyCoalesced;
}

/**
 * Chaitin-Briggs simplification: repeatedly remove low-degree virtual nodes onto the select
 * stack; when none remain, optimistically spill the node with the best cost/degree ratio.
 */
bool MirRegisterAllocator::simplify(RegisterAllocatorCtx *ctx)
{
    const uint32_t numNodes = static_cast<uint32_t>(ctx->m_idToReg.size());
    size_t totalVirtualNodes = 0;

    for (uint32_t id = 0; id < numNodes; ++id)
    {
        if (ctx->m_coalescedAway.test(id))
            continue;

        const MirRegisterRef &node = ctx->m_idToReg[id];
        if (node.isVirtual())
        {
            totalVirtualNodes++;
            MirRegisterClass *cls = node.getClass();
            if (!cls)
            {
                auto diag = ctx->m_ctx->getDiagCollector()->builder(Diag_Error, "MirRegisterAllocator");
                diag << "Virtual register has no assigned register class (Instruction selection rule missing "
                        "constraint)";
                diag.appendNote("{}", MirPrinter::printToString(node));
                return false;
            }
            (void)ctx->getColorLimit(cls);
        }
    }

    std::pmr::vector<uint32_t> lowDegreeQueue(ctx->m_allocator);
    lowDegreeQueue.reserve(totalVirtualNodes);

    std::pmr::vector<uint32_t> spillCandidates(ctx->m_allocator);
    spillCandidates.reserve(totalVirtualNodes);

    std::pmr::vector<double> rawCostCache(numNodes, 0.0, ctx->m_allocator);

    for (uint32_t id = 0; id < numNodes; ++id)
    {
        if (ctx->m_coalescedAway.test(id))
            continue;

        const MirRegisterRef &node = ctx->m_idToReg[id];
        if (!node.isVirtual())
            continue;

        const size_t K = ctx->m_classColorLimits[ctx->m_nodeClassIdx[id]];
        if (ctx->m_degree[id] < K)
        {
            lowDegreeQueue.push_back(id);
        }

        rawCostCache[id] = calculateSpillCost(node, ctx);
        if (!ctx->m_unspillableDense.test(id))
        {
            spillCandidates.push_back(id);
        }
    }

    while (ctx->m_selectStack.size() < totalVirtualNodes)
    {
        uint32_t candidate = RegisterAllocatorCtx::InvalidNode;

        // 1. Pop low-degree candidate
        while (!lowDegreeQueue.empty())
        {
            uint32_t nodeId = lowDegreeQueue.back();
            lowDegreeQueue.pop_back();

            if (!ctx->m_removed.test(nodeId))
            {
                candidate = nodeId;
                break;
            }
        }

        // 2. Chaitin optimistic spill candidate selection
        if (candidate == RegisterAllocatorCtx::InvalidNode)
        {
            double minCostRatio = std::numeric_limits<double>::max();
            size_t bestIdx = std::numeric_limits<size_t>::max();

            for (size_t i = 0; i < spillCandidates.size();)
            {
                const uint32_t nodeId = spillCandidates[i];
                if (ctx->m_removed.test(nodeId))
                {
                    spillCandidates[i] = spillCandidates.back();
                    spillCandidates.pop_back();
                    continue;
                }

                double deg = static_cast<double>(ctx->m_degree[nodeId]);
                double costRatio = rawCostCache[nodeId] / std::max(1.0, deg);

                if (costRatio < minCostRatio)
                {
                    minCostRatio = costRatio;
                    bestIdx = i;
                }
                ++i;
            }

            if (bestIdx < spillCandidates.size())
            {
                candidate = spillCandidates[bestIdx];
                spillCandidates[bestIdx] = spillCandidates.back();
                spillCandidates.pop_back();
            }
        }

        // 3. Fallback for unspillable constraints
        if (candidate == RegisterAllocatorCtx::InvalidNode)
        {
            for (uint32_t id = 0; id < numNodes; ++id)
            {
                if (!ctx->m_coalescedAway.test(id) && ctx->m_idToReg[id].isVirtual() && !ctx->m_removed.test(id))
                {
                    candidate = id;
                    break;
                }
            }
        }

        if (candidate == RegisterAllocatorCtx::InvalidNode)
        {
            break;
        }

        ctx->m_removed.set(candidate);
        ctx->m_selectStack.push_back(candidate);

        // Update active degree of neighbors and push newly simplified neighbors
        for (uint32_t neighbor : ctx->m_adjList[candidate])
        {
            if (!ctx->m_coalescedAway.test(neighbor) && ctx->m_idToReg[neighbor].isVirtual() &&
                !ctx->m_removed.test(neighbor))
            {
                if (ctx->m_degree[neighbor] > 0 && ctx->m_degree[neighbor] != RegisterAllocatorCtx::InfiniteDegree)
                {
                    uint32_t oldDeg = ctx->m_degree[neighbor];
                    ctx->m_degree[neighbor]--;

                    const size_t K = ctx->m_classColorLimits[ctx->m_nodeClassIdx[neighbor]];
                    if (oldDeg == K && ctx->m_degree[neighbor] < K)
                    {
                        lowDegreeQueue.push_back(neighbor);
                    }
                }
            }
        }
    }

    return true;
}

/**
 * Pops nodes off the select stack and assigns the first free physical color not taken by an
 * interfering neighbor; nodes with no color are recorded as spills. Returns true on success.
 */
bool MirRegisterAllocator::selectColors(RegisterAllocatorCtx *ctx)
{
    MirFunctionBuilder fBuilder(ctx->m_ctx);
    std::pmr::unordered_set<MirRegisterRef> spilledNodes(ctx->m_allocator);

    auto isSameBank = [](MirRegisterClass *c1, MirRegisterClass *c2)
    {
        if (!c1 || !c2)
            return true;
        return c1->getBank() == c2->getBank();
    };

    while (!ctx->m_selectStack.empty())
    {
        uint32_t nodeId = ctx->m_selectStack.back();
        ctx->m_selectStack.pop_back();
        const MirRegisterRef node = ctx->m_idToReg[nodeId];

        // Color ids are physical register indices within a class (16 on x86-64), so a per-node
        // bitmask replaces a heap-backed set and its hashing.
        std::bitset<64> usedColors;
        const auto markUsed = [&usedColors](size_t colorId)
        {
            if (colorId < usedColors.size())
                usedColors.set(colorId);
        };
        const auto isUsed = [&usedColors](size_t colorId)
        { return colorId < usedColors.size() && usedColors.test(colorId); };

        const auto &availableColors = node.getClass()->getRegs();

        // 1. Lock reserved registers
        for (const auto &[name, regDesc] : availableColors)
        {
            MirRegisterRef ref = MirRegisterRef::preg(regDesc);
            if (ctx->m_reservedRegs.contains(ref))
            {
                markUsed(ref.getId());
            }
        }

        // 2. Lock colors taken by interfering neighbors in the same register bank.
        // GPR and FPR families only compete within their own bank.
        for (uint32_t neighborId : ctx->m_adjList[nodeId])
        {
            if (ctx->m_removed.test(neighborId))
                continue;

            const MirRegisterRef &neighbor = ctx->m_idToReg[neighborId];
            if (neighbor.isPhysical())
            {
                if (isSameBank(neighbor.getClass(), node.getClass()))
                {
                    markUsed(neighbor.getId());
                }
            }
            else
            {
                auto it = ctx->m_allocatedRegs.find(neighbor);
                if (it != ctx->m_allocatedRegs.end())
                {
                    if (isSameBank(it->second.getClass(), node.getClass()))
                    {
                        markUsed(it->second.getId());
                    }
                }
            }
        }

        // 3. Search for available color
        std::optional<MirRegisterRef> assignedPhysReg;

        // Affinity-biased selection: prefer colors of coalescing/affinity partners
        if (nodeId < ctx->m_affinity.size())
        {
            for (uint32_t partnerId : ctx->m_affinity[nodeId])
            {
                uint32_t leaderId = ctx->getLeaderId(partnerId);
                const MirRegisterRef &leaderPartner = ctx->m_idToReg[leaderId];
                MirRegisterRef prefColor;
                if (leaderPartner.isPhysical())
                {
                    prefColor = leaderPartner;
                }
                else
                {
                    auto it = ctx->m_allocatedRegs.find(leaderPartner);
                    if (it != ctx->m_allocatedRegs.end())
                    {
                        prefColor = it->second;
                    }
                }

                if (prefColor.isPhysical() && prefColor.getClass() == node.getClass() && !isUsed(prefColor.getId()))
                {
                    assignedPhysReg = prefColor;
                    break;
                }
            }
        }

        if (!assignedPhysReg.has_value())
        {
            for (const auto &[name, regDesc] : availableColors)
            {
                MirRegisterRef ref = MirRegisterRef::preg(regDesc);
                if (!isUsed(ref.getId()))
                {
                    assignedPhysReg = ref;
                    break;
                }
            }
        }

        if (assignedPhysReg.has_value())
        {
            const auto &physRef = assignedPhysReg.value();

            if (CallingConvDesc *cc = ctx->m_targetFunction ? ctx->m_targetFunction->getCallingConv() : nullptr)
            {
                const auto &calleeSaved = cc->getCalleeSavedRegs(node.getClass());
                for (auto &reg : calleeSaved)
                {
                    if (isSamePhysReg(reg, physRef))
                    {
                        fBuilder.addPhysRegUse(ctx->m_targetFunction, reg);
                        break;
                    }
                }
            }

            ctx->m_allocatedRegs[node] = physRef;
            ctx->m_removed.clearBit(nodeId);
        }
        else
        {
            if (ctx->m_unspillableDense.test(nodeId) || ctx->m_unspillableRegs.contains(node))
            {
                auto log = ctx->m_ctx->getDiagCollector()->builder(Diag_Error, "MirRegisterAllocator");
                log << "Unspillable temporary register ran out of colors during select!";
                log.appendNote("Register: {}", MirPrinter::printToString(node));
                return false;
            }

            spilledNodes.insert(node);
        }
    }

    if (!spilledNodes.empty())
    {
        rewriteSpilledRegisters(spilledNodes, ctx);
        return false;
    }

    return true;
}

/**
 * Recomputes each node's degree and pins physical registers to infinite degree so they are
 * never simplified away.
 */
void MirRegisterAllocator::evaluateInterferenceGraphDegree(RegisterAllocatorCtx *ctx)
{
    const uint32_t numNodes = static_cast<uint32_t>(ctx->m_idToReg.size());
    for (uint32_t id = 0; id < numNodes; ++id)
    {
        if (ctx->m_coalescedAway.test(id))
            continue;

        const MirRegisterRef &node = ctx->m_idToReg[id];
        if (node.isPhysical())
        {
            ctx->m_degree[id] = RegisterAllocatorCtx::InfiniteDegree;
            ctx->m_allocatedRegs[node] = node;
            ctx->m_removed.clearBit(id);
        }
        else
        {
            ctx->m_degree[id] = static_cast<uint32_t>(ctx->m_adjList[id].size());
        }
    }
}

/**
 * Replaces every virtual register operand in the function with its assigned physical register.
 */
void MirRegisterAllocator::rewriteColors(RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            auto &operands = inst->getOperands();

            for (size_t i = 0; i < operands.size(); ++i)
            {
                auto rewriteReg = [&](MirRegister *reg)
                {
                    if (!reg)
                    {
                        return;
                    }
                    MirRegisterRef regRef = reg->getRef();
                    if (regRef.isVirtual())
                    {
                        MirRegisterRef leader = ctx->getCoalescedLeader(regRef);
                        if (leader.isPhysical())
                        {
                            reg->setRef(leader);
                        }
                        else
                        {
                            auto it = ctx->m_allocatedRegs.find(leader);
                            if (it != ctx->m_allocatedRegs.end())
                            {
                                reg->setRef(it->second);
                            }
                        }
                    }
                };

                if (operands[i]->isOfType<MirRegister>())
                {
                    rewriteReg(operands[i]->get<MirRegister>());
                }
                else if (auto *mem = operands[i]->get<MirMemory>())
                {
                    rewriteReg(mem->getBase());
                    rewriteReg(mem->getIndex());
                }
            }
        }
    }

    eliminateRedundantCopies(ctx);
}

/**
 * Erases redundant identity copy instructions (MOV r, r) produced by register coalescing and allocation.
 */
void MirRegisterAllocator::eliminateRedundantCopies(RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end();)
        {
            MirInstruction *inst = *it;
            ++it;

            bool isMove = (inst->getOpCode() == MirInstructionOpCode::MOV) ||
                          (inst->getTargetDesc() &&
                           (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsMove));
            if (!isMove)
            {
                continue;
            }

            if (inst->getOperandCount() < 2)
            {
                continue;
            }

            MirOperand *op0 = inst->getOperand(0);
            MirOperand *op1 = inst->getOperand(1);
            if (!op0 || !op1 || !op0->isOfType<MirRegister>() || !op1->isOfType<MirRegister>())
            {
                continue;
            }

            MirRegisterRef dst = op0->get<MirRegister>()->getRef();
            MirRegisterRef src = op1->get<MirRegister>()->getRef();

            if (dst == src)
            {
                MirInstructionBuilder(ctx->m_ctx, inst, InsertionType::InsertBefore).erase(inst);
            }
        }
    }
}

/**
 * Estimates how expensive it is to spill node by summing its definitions and uses.
 *
 * The per-register cost table is computed once per interference-graph build and reused across
 * simplification steps, so repeated candidate evaluations are O(1). Weights are flat (1) until
 * loop-depth analysis is available.
 */
double MirRegisterAllocator::calculateSpillCost(MirRegisterRef node, RegisterAllocatorCtx *ctx)
{
    if (!ctx->m_spillCostsValid)
    {
        ctx->m_spillCostsValid = true;
        ctx->m_spillCosts.clear();

        std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);
        std::pmr::vector<MirRegisterRef> uses(ctx->m_allocator);

        for (MirBlock *block : ctx->m_targetFunction->getBlocks())
        {
            for (MirInstruction *inst : block->getInstructions())
            {
                inst->getUsedRegisters(uses);
                for (const auto &use : uses)
                {
                    ctx->m_spillCosts[use] += 1.0;
                }

                inst->getDefinedRegisters(defs);
                for (const auto &def : defs)
                {
                    ctx->m_spillCosts[def] += 1.0;
                }
            }
        }
    }

    auto it = ctx->m_spillCosts.find(node);
    return it != ctx->m_spillCosts.end() ? it->second : 0.0;
}

/**
 * Ensures the dense interference graph has a node for v and returns its dense node ID.
 */
uint32_t MirRegisterAllocator::addNode(const MirRegisterRef &v, RegisterAllocatorCtx *ctx)
{
    return ctx->getOrAddNode(v);
}

/**
 * Rewrites the function for spilled registers: rematerializes cheap definitions, otherwise
 * creates stack slots and inserts reloads before / spills after each affected instruction.
 * Spill slots persist in the context across iterations so repeated runs converge.
 */
void MirRegisterAllocator::rewriteSpilledRegisters(const std::pmr::unordered_set<MirRegisterRef> &spilledNodes,
                                                   RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    MirFunctionStackFrame *stackFrame = func->getStackFrame();
    MirOperandBuilder opBuilder(ctx->m_ctx);

    // 1. Map defining instructions for rematerialization checks
    std::pmr::unordered_map<MirRegisterRef, MirInstruction *> definingInstMap(ctx->m_allocator);
    std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);
    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            inst->getDefinedRegisters(defs);
            for (const auto &def : defs)
            {
                if (def.isVirtual())
                    definingInstMap[def] = inst;
            }
        }
    }

    // 2. Pre-allocate stack spill slots
    for (const MirRegisterRef &spillRef : spilledNodes)
    {
        if (ctx->m_unspillableRegs.contains(spillRef) || ctx->m_spilledRegs.contains(spillRef))
            continue;

        MirRegister *vreg = ctx->m_ctx->getRegisterById(spillRef.getId());
        auto defIt = definingInstMap.find(spillRef);
        MirInstruction *defInst = (defIt != definingInstMap.end()) ? defIt->second : nullptr;

        if (!isRematerializable(vreg, defInst))
        {
            StackFrameObject *spillSlot = stackFrame->createStackSpill(vreg->getMirType());
            ctx->m_spilledRegs[spillRef] = spillSlot;
        }
    }

    // 3. Rewrite instruction operands
    for (MirBlock *block : func->getBlocks())
    {
        MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::Append);
        for (auto it = block->begin(); it != block->end(); it++)
        {
            MirInstruction *inst = *it;
            SourceReference *srcRef = inst->getSourceRef();
            auto &operands = inst->getOperands();

            // Writes must be spilled after the instruction executes, so they are queued here.
            struct DeferredSpill
            {
                MirRegister *tempReg;
                StackFrameObject *slot;
            };
            std::vector<DeferredSpill> postSpills;

            // Track rewritten registers for this instruction to prevent duplicate reload temporaries
            std::pmr::unordered_map<MirRegisterRef, MirRegister *> reloadedTemps(ctx->m_allocator);

            auto rewriteRegisterOp = [&](MirRegister *regOp, bool isRead, bool isWrite) -> MirRegister *
            {
                if (!regOp)
                    return nullptr;

                MirRegisterRef regRef = regOp->getRef();
                if (!spilledNodes.contains(regRef) || ctx->m_unspillableRegs.contains(regRef))
                    return nullptr;

                if (!isRead && !isWrite)
                    return nullptr;

                MirRegister *tempVReg = nullptr;
                auto cached = reloadedTemps.find(regRef);

                if (cached != reloadedTemps.end())
                {
                    tempVReg = cached->second;
                }
                else
                {
                    tempVReg = opBuilder.buildVReg(regOp->getMirType(), "spill_tmp", srcRef, regRef.getClass());
                    ctx->m_unspillableRegs.insert(tempVReg->getRef());
                    reloadedTemps[regRef] = tempVReg;

                    auto defIt = definingInstMap.find(regRef);
                    MirInstruction *defInst = (defIt != definingInstMap.end()) ? defIt->second : nullptr;
                    bool remat = isRematerializable(regOp, defInst);
                    StackFrameObject *slot = ctx->m_spilledRegs.contains(regRef) ? ctx->m_spilledRegs[regRef] : nullptr;

                    if (isRead)
                    {
                        if (remat)
                        {
                            auto log = ctx->m_ctx->getDiagCollector()->builder(Diag_Trace, "MirRegisterAllocator");
                            log << "Rematerializing register";
                            log.appendNote(srcRef, "Register: {}", MirPrinter::printToString(tempVReg));

                            reMaterialize(ctx, block, it, srcRef, tempVReg, defInst);
                        }
                        else if (slot)
                        {
                            emitReload(ctx, block, it, srcRef, tempVReg, slot);
                        }
                    }

                    if (isWrite && !remat && slot)
                    {
                        postSpills.push_back({ tempVReg, slot });
                    }
                }

                return tempVReg;
            };

            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (operands[i]->isOfType<MirRegister>())
                {
                    MirRegister *regOp = operands[i]->get<MirRegister>();
                    MirOperandFlag flags = inst->getOperandFlag(i);
                    bool isRead = (flags & MirOperandFlag::Read);
                    bool isWrite = (flags & MirOperandFlag::Write);

                    MirRegister *tempVReg = rewriteRegisterOp(regOp, isRead, isWrite);
                    if (tempVReg)
                    {
                        iBuilder.swapOperand(inst, tempVReg, i);
                    }
                }
                else if (auto *mem = operands[i]->get<MirMemory>())
                {
                    if (mem->hasBaseReg())
                    {
                        MirRegister *tempBase = rewriteRegisterOp(mem->getBase(), true, false);
                        if (tempBase)
                        {
                            mem->setBase(tempBase);
                        }
                    }
                    if (mem->hasIndexReg())
                    {
                        MirRegister *tempIndex = rewriteRegisterOp(mem->getIndex(), true, false);
                        if (tempIndex)
                        {
                            mem->setIndex(tempIndex);
                        }
                    }
                }
            }

            for (const auto &spill : postSpills)
            {
                emitSpill(ctx, block, it, srcRef, spill.slot, spill.tempReg);
            }
        }
    }
}