#include "TargetRegisterAllocator/RegisterAllocatorPass.h"

void InterferenceGraph::addNode(MirRegister *reg)
{
    if (m_nodes.find(reg) == m_nodes.end())
    {
        IGNode node{ reg, {}, -1, false };

        // CRITICAL Pre-color physical registers so they block virtual registers!
        if (!reg->isVirtual())
        {
            node.m_color = reg->getRegId();
        }

        m_nodes[reg] = node;
    }
}

void InterferenceGraph::addEdge(MirRegister *a, MirRegister *b)
{
    if (a == b)
        return;
    addNode(a);
    addNode(b);
    m_nodes[a].m_neighbors.insert(b);
    m_nodes[b].m_neighbors.insert(a);
}

IGNode &InterferenceGraph::getNode(MirRegister *reg) { return m_nodes[reg]; }
std::unordered_map<MirRegister *, IGNode> &InterferenceGraph::getNodes() { return m_nodes; }

void InterferenceGraph::clear() { m_nodes.clear(); }

RegisterAllocatorPass::RegisterAllocatorPass(RegisterAllocatorContext *ctx)
{
    m_ctx = ctx;
    m_targetDesc = ctx->getTargetDesc();
    m_emitter = ctx->getEmitter();
    m_emitterCtx = m_emitter->getContext();

    // Gather all allocatable registers from the ABI
    auto abi = m_targetDesc->getABI();
    m_allocatableRegs = abi->getCallerSavedRegs();
    auto calleeSaved = abi->getCalleeSavedRegs();
    m_allocatableRegs.insert(m_allocatableRegs.end(), calleeSaved.begin(), calleeSaved.end());

    m_k = m_allocatableRegs.size();
}

bool RegisterAllocatorPass::run(TypedPoolLinkedList<MirFunction> *funcList,
                                TypedPoolLinkedList<MirFunction>::Iterator it,
                                MirPassManager *passManager)
{
    MirFunction *func = *it;
    bool allocationComplete = false;

    while (!allocationComplete)
    {
        m_graph.clear();
        while (!m_selectStack.empty())
            m_selectStack.pop();

        const auto &la =
                passManager->getAnalysis<LivenessAnalysis>(func->getBlocks(), func->getBlocks()->begin(), m_emitter);
        const LivenessResult &liveness = la.getResult();

        // Build Graph
        buildGraph(func, liveness);

        // Simplify & Select
        simplifyAndSelect();

        // Check for Spills
        bool hasSpills = false;
        for (auto &pair : m_graph.getNodes())
        {
            if (pair.second.m_isSpilled)
            {
                hasSpills = true;
                break;
            }
        }

        if (hasSpills)
        {
            // Rewrite MIR (Insert Loads/Stores) and loop back
            rewriteProgram(func);
        }
        else
        {
            allocationComplete = true;
        }
    }

    // Final Assignment: Assign colors directly to the MIR instructions
    for (auto blockIt = func->getBlocks()->begin(); blockIt != func->getBlocks()->end(); ++blockIt)
    {
        for (MirInstruction *instr : *(*blockIt)->getInstructions())
        {
            auto operands = instr->getOperands();
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
            {
                MirOperand *op = *opIt;
                if (op->isOfType<MirRegister>())
                {
                    MirRegister *reg = op->get<MirRegister>();
                    if (reg->isVirtual())
                    {
                        int physicalId = m_graph.getNode(reg).m_color;

                        // Mutate the virtual register into a physical one
                        reg->setRegId(physicalId);
                        reg->setVirtual(false);
                    }
                }
            }
        }
    }

    return true;
}

void RegisterAllocatorPass::buildGraph(MirFunction *func, const LivenessResult &liveness)
{
    for (auto blockIt = func->getBlocks()->begin(); blockIt != func->getBlocks()->end(); ++blockIt)
    {
        MirBlock *block = *blockIt;
        size_t blockId = block->getId();

        // Initialize active live set with liveOut of this block
        std::unordered_set<MirRegister *> liveNow = liveness.m_liveOut.at(blockId);

        // Create a reversed array of instructions
        auto instrList = block->getInstructions();
        std::vector<MirInstruction *> reversedInstrs;
        for (auto instIt = instrList->begin(); instIt != instrList->end(); ++instIt)
        {
            reversedInstrs.push_back(*instIt);
        }
        std::reverse(reversedInstrs.begin(), reversedInstrs.end());

        for (MirInstruction *instr : reversedInstrs)
        {
            auto defs = getDefs(instr);
            auto uses = getUses(instr);

            // Add interference edges: defined regs interfere with all currently live regs
            for (MirRegister *def : defs)
            {
                m_graph.addNode(def);
                for (MirRegister *liveReg : liveNow)
                {
                    m_graph.addEdge(def, liveReg);
                }
            }

            // Step Liveness backwards
            for (MirRegister *def : defs)
                liveNow.erase(def); // Defs become dead moving backwards

            for (MirRegister *use : uses)
                liveNow.insert(use); // Uses become alive moving backwards
        }
    }
}

void RegisterAllocatorPass::simplifyAndSelect()
{
    auto nodes = m_graph.getNodes();
    std::unordered_set<MirRegister *> removedNodes;

    while (removedNodes.size() < nodes.size())
    {
        bool progress = false;

        // Try to find a node with degree < K that is NOT pre-colored
        for (auto &pair : nodes)
        {
            MirRegister *reg = pair.first;

            // Pre-colored physical registers have infinite degree. Never simplify them.
            if (!reg->isVirtual() || removedNodes.count(reg))
                continue;

            size_t activeDegree = 0;
            for (MirRegister *neighbor : pair.second.m_neighbors)
            {
                if (!removedNodes.count(neighbor))
                    activeDegree++;
            }

            if (activeDegree < m_k)
            {
                m_selectStack.push(reg);
                removedNodes.insert(reg);
                progress = true;
                break;
            }
        }

        // Optimistic Spilling: Pick the highest degree unremoved node
        if (!progress)
        {
            MirRegister *spillCandidate = nullptr;
            size_t maxDegree = 0;

            for (auto &pair : nodes)
            {
                MirRegister *reg = pair.first;
                if (!reg->isVirtual() || removedNodes.count(reg))
                    continue;

                size_t activeDegree = 0;
                for (MirRegister *neighbor : pair.second.m_neighbors)
                {
                    if (!removedNodes.count(neighbor))
                        activeDegree++;
                }

                if (activeDegree >= maxDegree)
                {
                    maxDegree = activeDegree;
                    spillCandidate = reg;
                }
            }

            if (spillCandidate)
            {
                m_selectStack.push(spillCandidate);
                removedNodes.insert(spillCandidate);
            }
            else
            {
                // The only nodes left are pre-colored physical registers. Mark them removed.
                for (auto &pair : nodes)
                    removedNodes.insert(pair.first);
            }
        }
    }

    while (!m_selectStack.empty())
    {
        MirRegister *reg = m_selectStack.top();
        m_selectStack.pop();

        IGNode &node = m_graph.getNode(reg);

        std::unordered_set<int> usedColors;
        for (MirRegister *neighborReg : node.m_neighbors)
        {
            int neighborColor = m_graph.getNode(neighborReg).m_color;
            if (neighborColor != -1)
                usedColors.insert(neighborColor);
        }

        bool assigned = false;
        for (PhysicalRegId physReg : m_allocatableRegs)
        {
            if (usedColors.find(physReg) == usedColors.end())
            {
                node.m_color = physReg;
                assigned = true;
                break;
            }
        }

        if (!assigned)
            node.m_isSpilled = true;
    }
}

bool RegisterAllocatorPass::rewriteProgram(MirFunction *func)
{
    std::unordered_map<MirRegister *, MirMemory *> spillMap;

    for (auto &pair : m_graph.getNodes())
    {
        if (pair.second.m_isSpilled)
        {
            MirRegister *spilledReg = pair.first;
            size_t size = spilledReg->getSizeInBytes();

            StackFrameObject *slot = func->getStackFrame()->createSpill(size, size);
            MirFrameIndex *frameIdx = m_emitter->createFrameIndex(spilledReg->getMirType(), slot->m_id);
            MirMemory *memAccess = m_emitter->createMemoryOperand(spilledReg->getMirType(), frameIdx, nullptr);

            spillMap[spilledReg] = memAccess;
        }
    }

    for (auto blockIt = func->getBlocks()->begin(); blockIt != func->getBlocks()->end(); ++blockIt)
    {
        MirBlock *block = *blockIt;
        auto instrList = block->getInstructions();

        auto instIt = instrList->begin();
        while (instIt != instrList->end())
        {
            // CRITICAL: Save the next original instruction NOW, before we insert anything.
            auto nextOriginalIt = instIt;
            ++nextOriginalIt;

            MirInstruction *instr = *instIt;
            auto operands = instr->getOperands();

            auto defs = getDefs(instr);
            auto uses = getUses(instr);

            // Track what we've replaced so we don't duplicate loads for 'ADD v1, v1'
            std::unordered_set<MirRegister *> replacedUses;

            // Handle Uses
            for (MirRegister *useReg : uses)
            {
                if (spillMap.contains(useReg) && !replacedUses.contains(useReg))
                {
                    replacedUses.insert(useReg);
                    m_emitterCtx->setInsertPoint(block, instIt);

                    MirRegister *newVReg = m_emitter->createVirtualRegister(useReg->getMirType());
                    m_emitter->emit(MirInstructionOpCode::LOAD, { newVReg, spillMap[useReg] });

                    for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
                    {
                        if (*opIt == useReg)
                            opIt.m_curr->m_object = newVReg;
                    }
                }
            }

            // Handle Defs
            std::unordered_set<MirRegister *> replacedDefs;
            for (MirRegister *defReg : defs)
            {
                if (spillMap.contains(defReg) && !replacedDefs.contains(defReg))
                {
                    replacedDefs.insert(defReg);
                    MirRegister *newVReg = m_emitter->createVirtualRegister(defReg->getMirType());

                    for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
                    {
                        if (*opIt == defReg)
                            opIt.m_curr->m_object = newVReg;
                    }

                    auto insertAfterIt = instIt;
                    ++insertAfterIt;
                    m_emitterCtx->setInsertPoint(block, insertAfterIt);

                    m_emitter->emit(MirInstructionOpCode::STORE, { spillMap[defReg], newVReg });
                }
            }

            m_emitterCtx->setInsertPoint(block);

            // CRITICAL: Jump safely to the next original instruction, skipping inserted STOREs.
            instIt = nextOriginalIt;
        }
    }

    return true;
}

const char *RegisterAllocatorPass::getName() const { return "RegisterAllocatorPass"; }

MirPassIterationPlace RegisterAllocatorPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

std::vector<MirRegister *> RegisterAllocatorPass::getDefs(class MirInstruction *instr)
{
    std::vector<MirRegister *> defs;
    auto operands = instr->getOperands();
    if (operands->m_numElems > 0)
    {
        MirOperand *op = operands->get<MirOperand>(0);
        if (op && op->isOfType<MirRegister>())
            defs.push_back(op->get<MirRegister>());
    }
    return defs;
}

std::vector<MirRegister *> RegisterAllocatorPass::getUses(class MirInstruction *instr)
{
    std::vector<MirRegister *> uses;
    auto operands = instr->getOperands();

    size_t i = 0;
    for (auto it = operands->begin(); it != operands->end(); ++it, ++i)
    {
        if (instr->getMetadata().m_operandConstraints[i].flags & OperandFlag::Write)
            continue; // Skip Dest

        MirOperand *op = *it;
        if (op && op->isOfType<MirRegister>())
        {
            uses.push_back(op->get<MirRegister>());
        }
        else if (op && op->isOfType<MirMemory>())
        {
            MirMemory *mem = op->get<MirMemory>();

            // Track the Base register
            if (mem->getBase() && mem->getBase()->isOfType<MirRegister>())
                uses.push_back(mem->getBase()->get<MirRegister>());
        }
    }
    return uses;
}
