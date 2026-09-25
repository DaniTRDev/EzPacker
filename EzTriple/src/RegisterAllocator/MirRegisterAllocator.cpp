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
#include "Printer/MirPrinter.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
#include <bitset>

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

    for (MirBlock *block : blockList)
    {
        size_t blockId = block->getId();
        const auto &liveOutSet = liveness->m_liveOut[blockId];

        std::pmr::unordered_set<MirRegisterRef> live(liveOutSet.begin(),
                                                     liveOutSet.end(),
                                                     liveOutSet.size(),
                                                     ctx->m_allocator);

        for (MirRegisterRef regRef : live)
        {
            addNode(regRef, ctx);
        }

        const auto &instructions = block->getInstructions();
        std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);
        std::pmr::vector<MirRegisterRef> uses(ctx->m_allocator);
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
                    (inst->getTargetDesc() && (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsCall));
            if (isCall)
            {
                analysisData->m_hasCalls = true;
                if (cc)
                {
                    const auto &callerSaved = cc->getAllCallerSavedRegs();
                    for (const auto &csReg : callerSaved)
                    {
                        // addNode returns the stable neighbor set, avoiding a re-hash per edge.
                        auto &csNeighbors = addNode(csReg, ctx);
                        for (const MirRegisterRef &liveRegRef : live)
                        {
                            if (csReg != liveRegRef)
                            {
                                csNeighbors.insert(liveRegRef);
                                addNode(liveRegRef, ctx).insert(csReg);
                            }
                        }
                    }
                }
            }

            // Add nodes and interference edges for DEFs
            for (const MirRegisterRef &defRegRef : defs)
            {
                auto &neighbors = addNode(defRegRef, ctx);

                for (const MirRegisterRef &liveRegRef : live)
                {
                    if (defRegRef != liveRegRef)
                    {
                        neighbors.insert(liveRegRef);
                        addNode(liveRegRef, ctx).insert(defRegRef);
                    }
                }
            }

            // In 2-address architectures (like x86-64), instructions dst = src1 OP src2 copy src1
            // into dst before the operation. If dst shares a physical register with src2 (operand index >= 2),
            // materializing the move would destroy src2 before it can be read.
            if (defs.size() == 1 && inst->getOperandCount() >= 3)
            {
                const MirRegisterRef &defReg = defs[0];
                for (size_t i = 2; i < inst->getOperandCount(); ++i)
                {
                    if (inst->getOperand(i) && inst->getOperand(i)->isOfType<MirRegister>())
                    {
                        MirRegisterRef srcReg = inst->getOperand(i)->get<MirRegister>()->getRef();
                        if (defReg != srcReg)
                        {
                            addNode(defReg, ctx).insert(srcReg);
                            addNode(srcReg, ctx).insert(defReg);
                        }
                    }
                }
            }

            // Erase DEFs from live set
            for (const MirRegisterRef &defRegRef : defs)
            {
                live.erase(defRegRef);
            }

            // Add USEs to live set
            for (const MirRegisterRef &useRegRef : uses)
            {
                addNode(useRegRef, ctx);
                live.insert(useRegRef);
            }
        }
    }

    // Reserve the frame and stack pointer registers, when the calling convention defines them.
    if (cc)
    {
        if (cc->hasFramePointer(func))
        {
            MirRegisterRef fpReg = cc->getFramePointerReg();
            ctx->m_reservedRegs.insert(fpReg);
        }

        // Reserve physical Stack Pointer register if physical
        MirRegisterRef spReg = cc->getStackPointerReg();
        if (spReg.isPhysical())
        {
            ctx->m_reservedRegs.insert(spReg);
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

    auto getColorLimit = [&](MirRegisterClass *cls) -> size_t
    {
        if (!cls)
        {
            return 0;
        }
        const auto &available = cls->getRegs();
        size_t usableCount = 0;
        for (auto &[name, regDesc] : available)
        {
            MirRegisterRef physReg = MirRegisterRef::preg(regDesc);
            if (!ctx->m_reservedRegs.contains(physReg))
            {
                usableCount++;
            }
        }
        return usableCount;
    };

    struct CoalesceCandidate
    {
        MirRegisterRef dst;
        MirRegisterRef src;
        bool isCopy;
    };

    std::pmr::vector<CoalesceCandidate> candidates(ctx->m_allocator);

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
                    MirRegisterRef dst = op0->get<MirRegister>()->getRef();
                    MirRegisterRef src = op1->get<MirRegister>()->getRef();
                    if (dst != src)
                    {
                        candidates.push_back({ dst, src, true });
                    }
                }
            }
            else if (inst->getOperandCount() >= 3)
            {
                std::pmr::vector<MirRegisterRef> defs(ctx->m_allocator);
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
                            candidates.push_back({ op0Ref, op1Ref, false });
                        }
                    }
                }
            }
        }
    }

    bool anyCoalesced = false;
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (const auto &cand : candidates)
        {
            MirRegisterRef u = ctx->getCoalescedLeader(cand.dst);
            MirRegisterRef v = ctx->getCoalescedLeader(cand.src);

            if (u == v)
            {
                continue;
            }

            // Ensure physical register is always 'u' if there is one
            if (u.isVirtual() && v.isPhysical())
            {
                std::swap(u, v);
            }

            // Cannot coalesce two distinct physical registers
            if (u.isPhysical() && v.isPhysical())
            {
                continue;
            }

            // Cannot coalesce reserved physical registers
            if (ctx->m_reservedRegs.contains(u) || ctx->m_reservedRegs.contains(v))
            {
                continue;
            }

            // Registers must belong to the same register class
            if (u.getClass() != v.getClass())
            {
                continue;
            }

            auto uIt = ctx->m_iGraph.find(u);
            auto vIt = ctx->m_iGraph.find(v);
            if (uIt == ctx->m_iGraph.end() || vIt == ctx->m_iGraph.end())
            {
                continue;
            }

            // If they interfere, they cannot be coalesced
            if (uIt->second.contains(v) || vIt->second.contains(u))
            {
                continue;
            }

            bool canCoalesce = false;

            if (u.isPhysical())
            {
                // George's criterion: for each neighbor t of v,
                // either t already interferes with u, or t is a low-degree virtual node (deg(t) < K_t)
                bool satisfiesGeorge = true;
                for (const MirRegisterRef &t : vIt->second)
                {
                    if (t == u)
                    {
                        continue;
                    }

                    if (uIt->second.contains(t))
                    {
                        continue;
                    }

                    if (t.isVirtual())
                    {
                        auto tIt = ctx->m_iGraph.find(t);
                        if (tIt != ctx->m_iGraph.end())
                        {
                            size_t degT = tIt->second.size();
                            size_t Kt = getColorLimit(t.getClass());
                            if (degT < Kt)
                            {
                                continue;
                            }
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
                size_t Ku = getColorLimit(u.getClass());

                std::pmr::unordered_set<MirRegisterRef> combinedNeighbors(ctx->m_allocator);
                for (const auto &n : uIt->second)
                {
                    if (n != u && n != v)
                    {
                        combinedNeighbors.insert(n);
                    }
                }
                for (const auto &n : vIt->second)
                {
                    if (n != u && n != v)
                    {
                        combinedNeighbors.insert(n);
                    }
                }

                size_t significantDegreeCount = 0;
                for (const auto &t : combinedNeighbors)
                {
                    if (t.isPhysical())
                    {
                        significantDegreeCount++;
                    }
                    else
                    {
                        auto tIt = ctx->m_iGraph.find(t);
                        if (tIt != ctx->m_iGraph.end())
                        {
                            size_t degT = tIt->second.size();
                            size_t Kt = getColorLimit(t.getClass());
                            if (degT >= Kt)
                            {
                                significantDegreeCount++;
                            }
                        }
                    }
                }

                canCoalesce = (significantDegreeCount < Ku);
            }

            if (canCoalesce)
            {
                // Coalesce v into u
                ctx->m_coalescedRegs[v] = u;

                if (u.isPhysical() && ctx->m_targetFunction)
                {
                    if (CallingConvDesc *cc = ctx->m_targetFunction->getCallingConv())
                    {
                        const auto &calleeSaved = cc->getCalleeSavedRegs(u.getClass());
                        for (auto &reg : calleeSaved)
                        {
                            if (reg == u)
                            {
                                MirFunctionBuilder fBuilder(ctx->m_ctx);
                                fBuilder.addPhysRegUse(ctx->m_targetFunction, reg);
                                break;
                            }
                        }
                    }
                }

                if (ctx->m_unspillableRegs.contains(v))
                {
                    ctx->m_unspillableRegs.insert(u);
                    ctx->m_unspillableRegs.erase(v);
                }

                // Merge interference edges of v into u
                auto neighborsOfV = vIt->second;
                for (const auto &w : neighborsOfV)
                {
                    if (w != u && w != v)
                    {
                        ctx->m_iGraph[u].insert(w);
                        ctx->m_iGraph[w].insert(u);
                        ctx->m_iGraph[w].erase(v);
                    }
                }

                ctx->m_iGraph[u].erase(v);
                ctx->m_iGraph.erase(v);

                anyCoalesced = true;
                changed = true;
                break; // Restart candidate scan with modified graph
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
    size_t totalVirtualNodes = 0;
    std::pmr::unordered_map<MirRegisterClass *, size_t> colorLimits(ctx->m_allocator);

    for (const auto &[node, neighbors] : ctx->m_iGraph)
    {
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

            if (!colorLimits.contains(cls))
            {
                const auto &available = cls->getRegs();
                size_t usableCount = 0;

                for (auto &[name, regDesc] : available)
                {
                    MirRegisterRef physReg = MirRegisterRef::preg(regDesc);
                    if (!ctx->m_reservedRegs.contains(physReg))
                    {
                        usableCount++;
                    }
                }
                colorLimits[cls] = usableCount;
            }
        }
    }

    std::pmr::vector<MirRegisterRef> lowDegreeQueue(ctx->m_allocator);
    lowDegreeQueue.reserve(totalVirtualNodes);

    for (const auto &[node, deg] : ctx->m_degree)
    {
        if (node.isVirtual() && deg < colorLimits[node.getClass()])
        {
            lowDegreeQueue.push_back(node);
        }
    }

    const auto &unspillable = ctx->m_unspillableRegs;

    while (ctx->m_selectStack.size() < totalVirtualNodes)
    {
        MirRegisterRef candidate;
        bool foundCandidate = false;

        // 1. Pop low-degree candidate
        while (!lowDegreeQueue.empty())
        {
            MirRegisterRef node = lowDegreeQueue.back();
            lowDegreeQueue.pop_back();

            if (!ctx->m_removedNodes.contains(node))
            {
                candidate = node;
                foundCandidate = true;
                break;
            }
        }

        // 2. Chaitin optimistic spill candidate selection
        if (!foundCandidate)
        {
            double minCostRatio = std::numeric_limits<double>::max();

            for (const auto &[node, neighbors] : ctx->m_iGraph)
            {
                if (node.isPhysical() || ctx->m_removedNodes.contains(node) || unspillable.contains(node))
                    continue;

                double deg = static_cast<double>(ctx->m_degree[node]);
                double rawCost = calculateSpillCost(node, ctx);
                double costRatio = rawCost / std::max(1.0, deg);

                if (costRatio < minCostRatio)
                {
                    minCostRatio = costRatio;
                    candidate = node;
                    foundCandidate = true;
                }
            }
        }

        // 3. Fallback for unspillable constraints
        if (!foundCandidate)
        {
            for (const auto &[node, neighbors] : ctx->m_iGraph)
            {
                if (node.isVirtual() && !ctx->m_removedNodes.contains(node))
                {
                    candidate = node;
                    foundCandidate = true;
                    break;
                }
            }
        }

        ctx->m_removedNodes.insert(candidate);
        ctx->m_selectStack.push_back(candidate);

        // Update active degree of neighbors and push newly simplified neighbors
        for (const MirRegisterRef &neighbor : ctx->m_iGraph[candidate])
        {
            if (neighbor.isVirtual() && !ctx->m_removedNodes.contains(neighbor))
            {
                if (ctx->m_degree[neighbor] > 0 && ctx->m_degree[neighbor] != std::numeric_limits<size_t>::max())
                {
                    size_t oldDeg = ctx->m_degree[neighbor];
                    ctx->m_degree[neighbor]--;

                    size_t K = colorLimits[neighbor.getClass()];
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

    while (!ctx->m_selectStack.empty())
    {
        MirRegisterRef node = ctx->m_selectStack.back();
        ctx->m_selectStack.pop_back();

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
        for (auto &[name, regDesc] : availableColors)
        {
            MirRegisterRef ref = MirRegisterRef::preg(regDesc);
            if (ctx->m_reservedRegs.contains(ref))
            {
                markUsed(ref.getId());
            }
        }

        // 2. Lock colors taken by interfering neighbors in the same register bank.
        // GPR and FPR families only compete within their own bank.
        auto isSameBank = [](MirRegisterClass *c1, MirRegisterClass *c2)
        {
            if (!c1 || !c2)
                return true;
            return c1->getBank() == c2->getBank();
        };

        for (const MirRegisterRef &neighbor : ctx->m_iGraph[node])
        {
            if (ctx->m_removedNodes.contains(neighbor))
                continue;

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
        auto affIt = ctx->m_affinity.find(node);
        if (affIt != ctx->m_affinity.end())
        {
            for (const MirRegisterRef &partner : affIt->second)
            {
                MirRegisterRef leaderPartner = ctx->getCoalescedLeader(partner);
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

                if (prefColor.isPhysical() &&
                    prefColor.getClass() == node.getClass() && !isUsed(prefColor.getId()))
                {
                    assignedPhysReg = prefColor;
                    break;
                }
            }
        }

        if (!assignedPhysReg.has_value())
        {
            for (auto &[name, regDesc] : availableColors)
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
                    if (reg == physRef)
                    {
                        fBuilder.addPhysRegUse(ctx->m_targetFunction, reg);
                        break;
                    }
                }
            }

            ctx->m_allocatedRegs[node] = physRef;
            ctx->m_removedNodes.erase(node);
        }
        else
        {
            if (ctx->m_unspillableRegs.contains(node))
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
    ctx->m_degree.clear();
    for (const auto &[node, neighbors] : ctx->m_iGraph)
    {
        if (node.isPhysical())
        {
            ctx->m_degree[node] = std::numeric_limits<size_t>::max();
            ctx->m_allocatedRegs[node] = node;
            ctx->m_removedNodes.erase(node);
        }
        else
        {
            ctx->m_degree[node] = neighbors.size();
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
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *regOp = operands[i]->get<MirRegister>();
                MirRegisterRef regRef = regOp->getRef();

                if (regRef.isVirtual())
                {
                    MirRegisterRef leader = ctx->getCoalescedLeader(regRef);
                    if (leader.isPhysical())
                    {
                        regOp->setRef(leader);
                    }
                    else
                    {
                        auto it = ctx->m_allocatedRegs.find(leader);
                        if (it != ctx->m_allocatedRegs.end())
                        {
                            regOp->setRef(it->second);
                        }
                    }
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
 * Ensures the interference graph has an (initially empty) node for v and returns its neighbor
 * set. The set is allocated from the allocator context (not the global arena) so the graph's
 * nodes and edges share one resource, and try_emplace guarantees a single hash per lookup.
 */
std::pmr::set<MirRegisterRef> &MirRegisterAllocator::addNode(const MirRegisterRef &v, RegisterAllocatorCtx *ctx)
{
    auto [it, inserted] = ctx->m_iGraph.try_emplace(v, std::pmr::set<MirRegisterRef>(ctx->m_allocator));
    (void)inserted;
    return it->second;
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

            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *regOp = operands[i]->get<MirRegister>();
                MirRegisterRef regRef = regOp->getRef();

                if (!spilledNodes.contains(regRef) || ctx->m_unspillableRegs.contains(regRef))
                    continue;

                MirOperandFlag flags = inst->getOperandFlag(i);
                bool isRead = (flags & MirOperandFlag::Read);
                bool isWrite = (flags & MirOperandFlag::Write);

                if (!isRead && !isWrite)
                    continue;

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

                iBuilder.swapOperand(inst, tempVReg, i);
            }

            for (const auto &spill : postSpills)
            {
                emitSpill(ctx, block, it, srcRef, spill.slot, spill.tempReg);
            }
        }
    }
}