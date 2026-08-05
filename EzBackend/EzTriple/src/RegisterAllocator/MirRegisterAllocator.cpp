#include "RegisterAllocator/MirRegisterAllocator.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <ranges>

bool MirRegisterAllocator::buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx &ctx)
{
    const auto &blockList = ctx.m_targetFunction->getBlocks();

    for (MirBlock *block : blockList)
    {
        size_t blockId = block->getId();
        const auto &liveOutSet = liveness->m_liveOut[blockId];

        std::pmr::unordered_set<RegisterRef> live(liveOutSet.begin(),
                                                  liveOutSet.end(),
                                                  liveOutSet.size(),
                                                  ctx.m_allocator);

        for (RegisterRef regRef : live)
        {
            addNode(regRef, ctx);
        }

        const auto &instructions = block->getInstructions();
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it)
        {
            MirInstruction *inst = *it;
            const auto &defs = inst->getDefinedRegisters();
            const auto &uses = inst->getUsedRegisters();

            // Add nodes and interference edges for DEFs
            for (const RegisterRef &defRegRef : defs)
            {
                addNode(defRegRef, ctx);
                auto &neighbors = ctx.m_iGraph[defRegRef];

                for (const RegisterRef &liveRegRef : live)
                {
                    if (defRegRef != liveRegRef)
                    {
                        neighbors.insert(liveRegRef);
                        ctx.m_iGraph[liveRegRef].insert(defRegRef);
                    }
                }
            }

            // Erase DEFs from live set
            for (const RegisterRef &defRegRef : defs)
            {
                live.erase(defRegRef);
            }

            // Add USEs to live set
            for (const RegisterRef &useRegRef : uses)
            {
                addNode(useRegRef, ctx);
                live.insert(useRegRef);
            }
        }
    }
    return true;
}

bool MirRegisterAllocator::simplify(RegisterAllocatorCtx &ctx)
{
    // Cache target register counts per class to avoid repetitive targetDesc lookups
    std::pmr::unordered_map<RegisterRefClass, size_t> colorLimits(ctx.m_allocator);
    size_t totalVirtualNodes = 0;

    for (const auto &[node, neighbors] : ctx.m_iGraph)
    {
        if (node.isVirtual())
        {
            totalVirtualNodes++;
            colorLimits.try_emplace(node.getClass(), ctx.m_targetDesc->getAvailableRegisters(node.getClass()).size());
        }
    }

    // Work queue for fast O(1) retrieval of low-degree nodes
    std::pmr::vector<RegisterRef> lowDegreeQueue(ctx.m_allocator);
    lowDegreeQueue.reserve(totalVirtualNodes);

    for (const auto &[node, deg] : ctx.m_degree)
    {
        if (node.isVirtual() && deg < colorLimits[node.getClass()])
        {
            lowDegreeQueue.push_back(node);
        }
    }

    const auto &unspillable = ctx.m_unspillableRegs;

    while (ctx.m_selectStack.size() < totalVirtualNodes)
    {
        RegisterRef candidate;
        bool foundCandidate = false;

        // O(1) Pop from low-degree work queue
        while (!lowDegreeQueue.empty())
        {
            RegisterRef node = lowDegreeQueue.back();
            lowDegreeQueue.pop_back();

            if (!ctx.m_removedNodes.contains(node))
            {
                candidate = node;
                foundCandidate = true;
                break;
            }
        }

        if (!foundCandidate)
        {
            double minCostRatio = std::numeric_limits<double>::max();

            for (const auto &[node, neighbors] : ctx.m_iGraph)
            {
                if (node.isPhysical() || ctx.m_removedNodes.contains(node) || unspillable.contains(node))
                    continue;

                double deg = static_cast<double>(ctx.m_degree[node]);
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

        // Emergency fallback for unspillable constraints
        if (!foundCandidate)
        {
            for (const auto &[node, neighbors] : ctx.m_iGraph)
            {
                if (node.isVirtual() && !ctx.m_removedNodes.contains(node))
                {
                    candidate = node;
                    foundCandidate = true;
                    break;
                }
            }
        }

        // Process removal
        ctx.m_removedNodes.insert(candidate);
        ctx.m_selectStack.push_back(candidate);

        // Update active degree of neighbors and push newly simplified neighbors to worklist
        for (const RegisterRef &neighbor : ctx.m_iGraph[candidate])
        {
            if (neighbor.isVirtual() && !ctx.m_removedNodes.contains(neighbor))
            {
                if (ctx.m_degree[neighbor] > 0 && ctx.m_degree[neighbor] != std::numeric_limits<size_t>::max())
                {
                    size_t oldDeg = ctx.m_degree[neighbor];
                    ctx.m_degree[neighbor]--;

                    size_t K = colorLimits[neighbor.getClass()];
                    if (oldDeg == K && ctx.m_degree[neighbor] < K)
                    {
                        lowDegreeQueue.push_back(neighbor);
                    }
                }
            }
        }
    }

    return true;
}

bool MirRegisterAllocator::selectColors(RegisterAllocatorCtx &ctx)
{
    std::pmr::unordered_set<RegisterRef> spilledNodes(ctx.m_allocator);
    std::vector<bool> usedColorsBitset;

    while (!ctx.m_selectStack.empty())
    {
        RegisterRef node = ctx.m_selectStack.back();
        ctx.m_selectStack.pop_back();

        const auto &availableColors = ctx.m_targetDesc->getAvailableRegisters(node.getClass());
        usedColorsBitset.assign(availableColors.size(), false);

        // Gather colors used by assigned neighbors
        for (const RegisterRef &neighbor : ctx.m_iGraph[node])
        {
            if (ctx.m_removedNodes.contains(neighbor))
                continue;

            auto it = ctx.m_allocatedRegs.find(neighbor);
            if (it != ctx.m_allocatedRegs.end())
            {
                const RegisterRef &assignedColor = it->second;
                for (size_t i = 0; i < availableColors.size(); ++i)
                {
                    if (availableColors[i] == assignedColor)
                    {
                        usedColorsBitset[i] = true;
                        break;
                    }
                }
            }
        }

        std::optional<RegisterRef> assignedPhysReg;
        for (size_t i = 0; i < availableColors.size(); ++i)
        {
            if (!usedColorsBitset[i])
            {
                assignedPhysReg = availableColors[i];
                break;
            }
        }

        if (assignedPhysReg.has_value())
        {
            const auto &calleeSaved = ctx.m_targetFunction->getCallingConv()->getCalleeSavedRegs(node.getClass());
            const auto &physRef = assignedPhysReg.value();

            for (auto &reg : calleeSaved)
            {
                if (reg == physRef)
                {
                    ctx.m_targetFunction->addCalleeSavedRegUse(reg);
                    break;
                }
            }

            ctx.m_allocatedRegs[node] = physRef;
            ctx.m_removedNodes.erase(node);
        }
        else
        {
            if (ctx.m_unspillableRegs.contains(node))
            {
                auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Error, "MirRegisterAllocator");
                log << "Unspillable temporary register ran out of colors during select!";
                log.appendNote(std::format("Register: {}", MirPrinter::printToString(node)).c_str(), nullptr);
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

void MirRegisterAllocator::evaluateInterferenceGraphDegree(RegisterAllocatorCtx &ctx)
{
    ctx.m_degree.clear();
    for (const auto &[node, neighbors] : ctx.m_iGraph)
    {
        if (node.isPhysical())
        {
            ctx.m_degree[node] = std::numeric_limits<size_t>::max();
            ctx.m_allocatedRegs[node] = node;
        }
        else
        {
            ctx.m_degree[node] = neighbors.size();
        }
    }
}

void MirRegisterAllocator::rewriteColors(RegisterAllocatorCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunction;
    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            bool modified = false;
            auto &operands = inst->getOperands();

            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *regOp = operands[i]->get<MirRegister>();
                RegisterRef regRef = regOp->getRef();

                if (regRef.isVirtual())
                {
                    auto it = ctx.m_allocatedRegs.find(regRef);
                    if (it != ctx.m_allocatedRegs.end())
                    {
                        regOp->setRef(it->second);
                        modified = true;
                    }
                }
            }

            if (modified)
            {
                inst->invalidateCachedUsedAndDefs();
            }
        }
    }
}

bool MirRegisterAllocator::isRematerializable(MirRegister *vreg, MirInstruction *definingInst)
{
    if (!vreg || !definingInst)
        return false;

    if (definingInst->getOpCode() == MOV)
    {
        const auto &operands = definingInst->getOperands();
        if (operands.size() >= 2)
        {
            MirOperand *src = operands[1];
            return src->isOfType<MirInteger>() || src->isOfType<MirFloat>();
        }
    }

    return false;
}

double MirRegisterAllocator::calculateSpillCost(RegisterRef node, RegisterAllocatorCtx &ctx)
{
    double totalCost = 0.0;

    for (MirBlock *block : ctx.m_targetFunction->getBlocks())
    {
        size_t loopDepth = 0; // TODO: Populated by LoopAnalysis pass
        double weight = std::pow(10.0, static_cast<double>(loopDepth));

        for (MirInstruction *inst : block->getInstructions())
        {
            for (const auto &use : inst->getUsedRegisters())
            {
                if (use == node)
                    totalCost += 1.0 * weight;
            }
            for (const auto &def : inst->getDefinedRegisters())
            {
                if (def == node)
                    totalCost += 1.0 * weight;
            }
        }
    }

    return totalCost;
}

void MirRegisterAllocator::addEdge(const RegisterRef &u, const RegisterRef &v, RegisterAllocatorCtx &ctx)
{
    if (u == v)
        return;

    ctx.m_iGraph[u].insert(v);
    ctx.m_iGraph[v].insert(u);
}

void MirRegisterAllocator::addNode(const RegisterRef &v, RegisterAllocatorCtx &ctx)
{
    ctx.m_iGraph.try_emplace(v, std::pmr::set<RegisterRef>(ctx.m_ctx->getGlobalAllocator()));
}

void MirRegisterAllocator::rewriteSpilledRegisters(const std::pmr::unordered_set<RegisterRef> &spilledNodes,
                                                   RegisterAllocatorCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunction;
    MirFunctionStackFrame *stackFrame = func->getStackFrame();

    // Map defining instructions for virtual registers to evaluate rematerialization
    std::pmr::unordered_map<RegisterRef, MirInstruction *> definingInstMap(ctx.m_allocator);
    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            for (const auto &def : inst->getDefinedRegisters())
            {
                if (def.isVirtual())
                {
                    definingInstMap[def] = inst;
                }
            }
        }
    }

    // Allocate stack spill slots ONLY for non-rematerializable spilled registers
    for (const RegisterRef &spillRegRef : spilledNodes)
    {
        if (ctx.m_spilledRegs.contains(spillRegRef))
            continue;

        MirRegister *vreg = ctx.m_ctx->getRegisterById(spillRegRef.getId());
        MirInstruction *defInst = definingInstMap[spillRegRef];

        if (!isRematerializable(vreg, defInst))
        {
            MirType *regType = vreg->getMirType();
            StackFrameObject *spillSlot = stackFrame->createStackSpill(regType);
            ctx.m_spilledRegs[spillRegRef] = spillSlot;
        }
    }

    for (MirBlock *block : func->getBlocks())
    {
        auto &origInstructions = block->getInstructions();
        for (auto it = origInstructions.begin(); it != origInstructions.end(); ++it)
        {
            MirInstruction *inst = *it;
            const auto &operandConsts = inst->getMetadata().m_operandConstraints;
            SourceReference *srcRef = inst->getSourceRef();
            MirOperandBuilder opBuilder(ctx.m_ctx);

            auto &operands = inst->getOperands();

            // --- USES (LOAD / REMATERIALIZE) ---
            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *usedReg = operands[i]->get<MirRegister>();
                RegisterRef usedRef = usedReg->getRef();

                if (spilledNodes.contains(usedRef))
                {
                    if (i >= operandConsts.size() || (operandConsts[i].flags & OperandFlag::Read) == 0)
                        continue;

                    MirType *regType = usedReg->getMirType();
                    MirRegister *reloadVReg = opBuilder.buildVReg(regType, "spill_reload", srcRef);
                    MirInstructionBuilder insertBeforeBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, it);

                    MirInstruction *defInst = definingInstMap[usedRef];
                    if (isRematerializable(usedReg, defInst))
                    {
                        // Rematerialization: Re-execute the immediate MOV inline instead of reading memory!
                        MirOperand *constVal = defInst->getOperands()[1];
                        MirInstruction *rematInst = insertBeforeBuilder.MOV(srcRef, reloadVReg, constVal);
                        rematInst->invalidateCachedUsedAndDefs();
                    }
                    else
                    {
                        // Standard Spill Reload from Stack
                        StackFrameObject *spillSlot = ctx.m_spilledRegs[usedRef];
                        MirReference *spillSlotRef = opBuilder.buildRef(spillSlot, srcRef);

                        MirInstruction *loadInst = insertBeforeBuilder.LOAD(srcRef, reloadVReg, spillSlotRef);
                        loadInst->invalidateCachedUsedAndDefs();
                    }

                    operands[i] = reloadVReg;
                    ctx.m_unspillableRegs.insert(reloadVReg->getRef());
                }
            }

            // --- DEFS (STORE / REMATERIALIZATION BYPASS) ---
            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *defReg = operands[i]->get<MirRegister>();
                RegisterRef defRef = defReg->getRef();

                if (spilledNodes.contains(defRef))
                {
                    if (i >= operandConsts.size() || (operandConsts[i].flags & OperandFlag::Write) == 0)
                        continue;

                    MirInstruction *defInst = definingInstMap[defRef];
                    if (isRematerializable(defReg, defInst))
                    {
                        // Skip emitting STORE to stack memory completely for rematerializable constants!
                        continue;
                    }

                    StackFrameObject *spillSlot = ctx.m_spilledRegs[defRef];
                    MirType *regType = defReg->getMirType();

                    MirRegister *spillVReg = opBuilder.buildVReg(regType, "spill_def", srcRef);
                    MirReference *spillSlotRef = opBuilder.buildRef(spillSlot, srcRef);

                    operands[i] = spillVReg;
                    ctx.m_unspillableRegs.insert(spillVReg->getRef());

                    MirInstructionBuilder insertAfterBuilder(ctx.m_ctx, block, InsertionType::InsertAfter, it);
                    MirInstruction *storeInst = insertAfterBuilder.STORE(srcRef, spillSlotRef, spillVReg);
                    storeInst->invalidateCachedUsedAndDefs();
                }
            }

            inst->invalidateCachedUsedAndDefs();
        }
    }
}