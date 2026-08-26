#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Printer/MirPrinter.h"
#include "RegisterAllocator/MirRegisterAllocator.h"

bool MirRegisterAllocator::buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    CallingConvDesc *cc = func->getCallingConv();
    const auto analysisData = func->getAnalysisData();
    const auto &blockList = func->getBlocks();

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
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it)
        {
            MirInstruction *inst = *it;
            const auto &defs = inst->getDefinedRegisters();
            const auto &uses = inst->getUsedRegisters();

            if (isInstructionDAlloc(inst))
            {
                analysisData->m_hasDynamicAllocs = true;
            }

            // Add nodes and interference edges for DEFs
            for (const MirRegisterRef &defRegRef : defs)
            {
                addNode(defRegRef, ctx);
                auto &neighbors = ctx->m_iGraph[defRegRef];

                for (const MirRegisterRef &liveRegRef : live)
                {
                    if (defRegRef != liveRegRef)
                    {
                        neighbors.insert(liveRegRef);
                        ctx->m_iGraph[liveRegRef].insert(defRegRef);
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

    // Reserve physical Frame Pointer register if required
    if (cc->hasFramePointer(func))
    {
        MirRegisterRef fpReg = cc->getFramePointerReg();
        ctx->m_reservedRegs.insert(fpReg);
    }

    return true;
}

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

bool MirRegisterAllocator::selectColors(RegisterAllocatorCtx *ctx)
{
    std::pmr::unordered_set<MirRegisterRef> spilledNodes(ctx->m_allocator);

    while (!ctx->m_selectStack.empty())
    {
        MirRegisterRef node = ctx->m_selectStack.back();
        ctx->m_selectStack.pop_back();

        // Fresh set for every individual node being colored
        std::pmr::unordered_set<MirRegisterRef> usedColorsSet(ctx->m_allocator);

        const auto &availableColors = node.getClass()->getRegs();

        // 1. Lock reserved registers
        for (auto &[name, regDesc] : availableColors)
        {
            MirRegisterRef ref = MirRegisterRef::preg(regDesc);
            if (ctx->m_reservedRegs.contains(ref))
            {
                usedColorsSet.insert(ref);
            }
        }

        // 2. Lock colors taken by interfering neighbors
        for (const MirRegisterRef &neighbor : ctx->m_iGraph[node])
        {
            if (ctx->m_removedNodes.contains(neighbor))
                continue;

            auto it = ctx->m_allocatedRegs.find(neighbor);
            if (it != ctx->m_allocatedRegs.end())
            {
                usedColorsSet.insert(it->second);
            }
        }

        // 3. Search for available color
        std::optional<MirRegisterRef> assignedPhysReg;
        for (auto &[name, regDesc] : availableColors)
        {
            MirRegisterRef ref = MirRegisterRef::preg(regDesc);
            if (!usedColorsSet.contains(ref))
            {
                assignedPhysReg = ref;
                break;
            }
        }

        if (assignedPhysReg.has_value())
        {
            const auto &calleeSaved = ctx->m_targetFunction->getCallingConv()->getCalleeSavedRegs(node.getClass());
            const auto &physRef = assignedPhysReg.value();

            for (auto &reg : calleeSaved)
            {
                if (reg == physRef)
                {
                    ctx->m_targetFunction->addCalleeSavedRegUse(reg);
                    break;
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

void MirRegisterAllocator::rewriteColors(RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
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
                MirRegisterRef regRef = regOp->getRef();

                if (regRef.isVirtual())
                {
                    auto it = ctx->m_allocatedRegs.find(regRef);
                    if (it != ctx->m_allocatedRegs.end())
                    {
                        regOp->setRef(it->second);
                        modified = true;
                    }
                }
            }
        }
    }
}

double MirRegisterAllocator::calculateSpillCost(MirRegisterRef node, RegisterAllocatorCtx *ctx)
{
    double totalCost = 0.0;

    for (MirBlock *block : ctx->m_targetFunction->getBlocks())
    {
        size_t loopDepth = 0;
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

void MirRegisterAllocator::addEdge(const MirRegisterRef &u, const MirRegisterRef &v, RegisterAllocatorCtx *ctx)
{
    if (u == v)
        return;

    ctx->m_iGraph[u].insert(v);
    ctx->m_iGraph[v].insert(u);
}

void MirRegisterAllocator::addNode(const MirRegisterRef &v, RegisterAllocatorCtx *ctx)
{
    ctx->m_iGraph.try_emplace(v, std::pmr::set<MirRegisterRef>(ctx->m_ctx->getGlobalAllocator()));
}

void MirRegisterAllocator::rewriteSpilledRegisters(const std::pmr::unordered_set<MirRegisterRef> &spilledNodes,
                                                   RegisterAllocatorCtx *ctx)
{
    MirFunction *func = ctx->m_targetFunction;
    MirFunctionStackFrame *stackFrame = func->getStackFrame();
    MirOperandBuilder opBuilder(ctx->m_ctx);

    // 1. Map defining instructions for rematerialization checks
    std::pmr::unordered_map<MirRegisterRef, MirInstruction *> definingInstMap(ctx->m_allocator);
    for (MirBlock *block : func->getBlocks())
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            for (const auto &def : inst->getDefinedRegisters())
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
        std::vector<MirInstruction *> origInstructions(block->getInstructions().begin(),
                                                       block->getInstructions().end());

        for (MirInstruction *inst : origInstructions)
        {
            auto it = std::find(block->getInstructions().begin(), block->getInstructions().end(), inst);
            if (it == block->getInstructions().end())
                continue;

            SourceReference *srcRef = inst->getSourceRef();
            auto &operands = inst->getOperands();

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

                operands[i] = tempVReg;
            }

            for (const auto &spill : postSpills)
            {
                emitSpill(ctx, block, it, srcRef, spill.slot, spill.tempReg);
            }
        }
    }
}