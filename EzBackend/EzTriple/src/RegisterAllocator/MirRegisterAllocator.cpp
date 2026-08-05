#include "RegisterAllocator/MirRegisterAllocator.h"

#include <ranges>

bool MirRegisterAllocator::buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx &ctx)
{
    const auto &blockList = ctx.m_targetFunction->getBlocks();
    MirFunction *targetFunc = ctx.m_targetFunction;

    for (MirBlock *block : blockList)
    {
        size_t blockId = block->getId();
        std::pmr::unordered_set<RegisterRef> live = liveness->m_liveOut[blockId];

        // Ensure all registers live at the exit of the block have nodes in G
        for (RegisterRef regRef : live)
        {
            addNode(regRef, ctx);
        }

        // Iterate backwards through instructions in the block
        const auto &instructions = block->getInstructions();
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it)
        {
            MirInstruction *inst = *it;

            // Extract defined and used registers for this instruction
            // (Replace these calls with your actual instruction API, e.g., inst->getDefs(), inst->getUses())
            std::pmr::vector<RegisterRef> defs = inst->getDefinedRegisters();
            const std::pmr::vector<RegisterRef> &uses = inst->getUsedRegisters();

            // Check if instruction is a register-to-register MOVE (u = v)
            const auto &operands = inst->getOperands();

            bool isMove = (inst->getOpCode() == MOV);
            bool isCall = inst->getOpCode() == CALL;

            if (isCall)
            {
                for (size_t i = static_cast<uint8_t>(RegisterRefClass::Invalid) + 1;
                     i < static_cast<uint8_t>(RegisterRefClass::MAX_REF_TYPE);
                     i++)
                {
                    const auto &callerSavedRegs =
                            targetFunc->getCallingConv()->getCallerSavedRegs(static_cast<RegisterRefClass>(i));

                    for (auto &reg : callerSavedRegs)
                        defs.push_back(reg);
                }
            }

            // Add interferences for defined registers
            for (const auto &defRegRef : defs)
            {
                addNode(defRegRef, ctx);

                for (const auto &liveRegRef : live)
                {
                    // Special case for MOVE inst (u <- v): u and v do NOT interfere simply because
                    // v is live at the def of u.
                    if (isMove)
                    {
                        MirRegister *src = operands[1]->get<MirRegister>();
                        if (src && liveRegRef == src->getRef())
                            continue;
                    }

                    addEdge(defRegRef, liveRegRef, ctx);
                }
            }

            // Update live set for preceding instructions:
            // Remove DEFs (they are dead before this point unless used previously)
            for (const auto &defRegRef : defs)
            {
                live.erase(defRegRef);
            }

            // Add USEs (they become live before this instruction)
            for (const auto &useRegRef : uses)
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
    size_t totalVirtualNodes = 0;
    for (const auto &node : ctx.m_iGraph | std::views::keys)
    {
        if (node.isVirtual())
            totalVirtualNodes++;
    }

    const auto &unspillable = ctx.m_unspillableRegs;

    while (ctx.m_selectStack.size() < totalVirtualNodes)
    {
        RegisterRef candidate;
        bool foundCandidate = false;

        // Try to find ANY virtual node with degree < K (including unspillable ones)
        for (const auto &node : ctx.m_iGraph | std::views::keys)
        {
            if (node.isPhysical() || ctx.m_removedNodes.contains(node))
                continue;

            const auto &availableColors = ctx.m_targetDesc->getAvailableRegisters(node.getClass());
            if (ctx.m_degree[node] < availableColors.size())
            {
                candidate = node;
                foundCandidate = true;
                break;
            }
        }

        // Chaitin-Briggs Optimistic Spill: If no node has degree < K,
        // Select a spill candidate ONLY from SPILLABLE nodes (exclude unspillable)
        if (!foundCandidate)
        {
            double minCost = std::numeric_limits<double>::max();

            for (const auto &node : ctx.m_iGraph | std::views::keys)
            {
                if (node.isPhysical() || ctx.m_removedNodes.contains(node) || unspillable.contains(node))
                    continue; // Skip unspillable nodes ONLY during forced spill selection

                double degree = static_cast<double>(ctx.m_degree[node]);
                double staticSpillCost = 1.0;
                double cost = staticSpillCost / std::max(1.0, degree);

                if (cost < minCost)
                {
                    minCost = cost;
                    candidate = node;
                    foundCandidate = true;
                }
            }
        }

        // Emergency fallback: If constrained by unspillable nodes with degree >= K,
        // force simplify them to avoid deadlocking the compiler.
        if (!foundCandidate)
        {
            for (const auto &node : ctx.m_iGraph | std::views::keys)
            {
                if (!node.isPhysical() && !ctx.m_removedNodes.contains(node))
                {
                    candidate = node;
                    foundCandidate = true;
                    break;
                }
            }
        }

        // Remove selected candidate and push onto select stack
        ctx.m_removedNodes.insert(candidate);
        ctx.m_selectStack.push_back(candidate);

        // Update neighbors' active degree
        for (const RegisterRef &neighbor : ctx.m_iGraph[candidate])
        {
            if (!ctx.m_removedNodes.contains(neighbor))
            {
                if (ctx.m_degree[neighbor] > 0 && ctx.m_degree[neighbor] != std::numeric_limits<size_t>::max())
                {
                    ctx.m_degree[neighbor]--;
                }
            }
        }
    }

    return true;
}

bool MirRegisterAllocator::selectColors(RegisterAllocatorCtx &ctx)
{
    std::pmr::unordered_set<RegisterRef> spilledNodes(ctx.m_ctx->getGlobalAllocator());

    // Pop nodes off the select stack in reverse order of removal
    while (!ctx.m_selectStack.empty())
    {
        RegisterRef node = ctx.m_selectStack.back();
        ctx.m_selectStack.pop_back();

        std::pmr::unordered_set<RegisterRef> usedColors(ctx.m_ctx->getGlobalAllocator());

        // Gather physical colors used by active neighbors
        for (const RegisterRef &neighbor : ctx.m_iGraph[node])
        {
            if (ctx.m_removedNodes.contains(neighbor))
                continue; // Neighbor hasn't been assigned or is removed

            auto it = ctx.m_allocatedRegs.find(neighbor);
            if (it != ctx.m_allocatedRegs.end())
            {
                usedColors.insert(it->second);
            }
        }

        const auto &availableColors = ctx.m_targetDesc->getAvailableRegisters(node.getClass());

        std::optional<RegisterRef> assignedPhysReg;
        for (const auto &physReg : availableColors)
        {
            if (!usedColors.contains(physReg))
            {
                assignedPhysReg = physReg;
                break;
            }
        }

        if (assignedPhysReg.has_value())
        {
            const auto &physRes = assignedPhysReg.value();
            ctx.m_allocatedRegs[node] = physRes;
            ctx.m_removedNodes.erase(node);
        }
        else
        {
            // Optimistic coloring failed — node must be spilled to stack memory
            auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirRegisterAllocator");
            log << "Spilling register to stack";
            log.appendNote(std::format("Spilled register: {}", MirPrinter::printToString(node)).c_str(), nullptr);

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
            // Pre-colored physical nodes have infinite degree so they are never removed
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
    MirOperandBuilder opBuilder(ctx.m_ctx);

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
                RegisterRef regRef = regOp->getRef();

                // If this is a virtual register, replace operand with physical register singleton
                if (regRef.isVirtual())
                {
                    auto it = ctx.m_allocatedRegs.find(regRef);
                    if (it != ctx.m_allocatedRegs.end())
                    {
                        RegisterRef physRef = it->second;

                        auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirRegisterAllocator");
                        log << "Allocated physical register";
                        log.appendNote(std::format("Virtual register: {}", MirPrinter::printToString(regRef)).c_str(),
                                       nullptr);
                        log.appendNote(std::format("Physical register: {}", MirPrinter::printToString(physRef)).c_str(),
                                       nullptr);

                        // Mutate or replace virtual register operand with physical register handle
                        regOp->setRef(physRef);
                        inst->invalidateCachedUsedAndDefs();
                    }
                }
            }
        }
    }
}

void MirRegisterAllocator::addEdge(const RegisterRef &u, const RegisterRef &v, RegisterAllocatorCtx &ctx)
{
    if (u == v)
        return; // Prevent self-loops

    addNode(u, ctx);
    addNode(v, ctx);

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

    for (const RegisterRef &spillRegRef : spilledNodes)
    {
        if (ctx.m_spilledRegs.contains(spillRegRef))
            continue;

        MirRegister *vreg = ctx.m_ctx->getRegisterById(spillRegRef.getId());
        MirType *regType = vreg->getMirType();
        StackFrameObject *spillSlot = stackFrame->createStackSpill(regType);
        ctx.m_spilledRegs[spillRegRef] = spillSlot;
    }

    for (MirBlock *block : func->getBlocks())
    {
        // Snapshot the original instruction array so insertions don't disrupt iteration
        auto origInstructions = block->getInstructions();

        for (MirInstruction *inst : origInstructions)
        {
            const auto &operandConsts = inst->getMetadata().m_operandConstraints;
            SourceReference *srcRef = inst->getSourceRef();
            MirOperandBuilder opBuilder(ctx.m_ctx);

            // Locate current iterator position inside the live block instructions
            auto &instructions = block->getInstructions();
            auto instIt = std::find(instructions.begin(), instructions.end(), inst);
            if (instIt == instructions.end())
                continue;

            auto &operands = inst->getOperands();

            // --- HANDLE USES (LOAD) ---
            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *usedReg = operands[i]->get<MirRegister>();
                RegisterRef usedRef = usedReg->getRef();

                if (spilledNodes.contains(usedRef))
                {
                    if ((operandConsts[i].flags & OperandFlag::Read) == 0)
                        continue;

                    StackFrameObject *spillSlot = ctx.m_spilledRegs[usedRef];
                    MirType *regType = usedReg->getMirType();

                    MirRegister *reloadVReg = opBuilder.buildVReg(regType, "spill_reload", srcRef);
                    MirReference *spillSlotRef = opBuilder.buildRef(spillSlot, srcRef);

                    MirInstructionBuilder insertBeforeBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, instIt);
                    insertBeforeBuilder.LOAD(srcRef, reloadVReg, spillSlotRef);

                    operands[i] = reloadVReg;
                    inst->invalidateCachedUsedAndDefs();
                }
            }

            // --- HANDLE DEFS (STORE) ---
            for (size_t i = 0; i < operands.size(); ++i)
            {
                if (!operands[i]->isOfType<MirRegister>())
                    continue;

                MirRegister *defReg = operands[i]->get<MirRegister>();
                RegisterRef defRef = defReg->getRef();

                if (spilledNodes.contains(defRef))
                {
                    if ((operandConsts[i].flags & OperandFlag::Write) == 0)
                        continue;

                    StackFrameObject *spillSlot = ctx.m_spilledRegs[defRef];
                    MirType *regType = defReg->getMirType();

                    MirRegister *spillVReg = opBuilder.buildVReg(regType, "spill_def", srcRef);
                    MirReference *spillSlotRef = opBuilder.buildRef(spillSlot, srcRef);

                    operands[i] = spillVReg;
                    inst->invalidateCachedUsedAndDefs();

                    MirInstructionBuilder insertAfterBuilder(ctx.m_ctx, block, InsertionType::InsertAfter, instIt);
                    insertAfterBuilder.STORE(srcRef, spillSlotRef, spillVReg);
                }
            }
        }
    }
}