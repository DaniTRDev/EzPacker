#include "MirPass/Passes/LivenessAnalysis.h"
#include "MirPass/MirPassManager.h"

LivenessAnalysis::LivenessAnalysis(MirEmitter *emitter) : m_emitter(emitter) {}

bool LivenessAnalysis::run(TypedPoolLinkedList<class MirBlock> *blockList,
                           TypedPoolLinkedList<class MirBlock>::Iterator it,
                           class MirPassManager *passManager)
{
    m_result.m_liveIn.clear();
    m_result.m_liveOut.clear();
    m_result.m_def.clear();
    m_result.m_use.clear();

    auto &cfgPass = passManager->getAnalysis<CodeFlowAnalysis>(blockList, it, m_emitter);
    const ControlFlowResult &cfg = cfgPass.getResult();

    computeLocalLiveness(blockList, it);
    computeGlobalLiveness(blockList, it, cfg);

    return true;
}

const char *LivenessAnalysis::getName() const { return "LivenessAnalysis"; }

const LivenessResult &LivenessAnalysis::getResult() const { return m_result; }

void LivenessAnalysis::computeLocalLiveness(TypedPoolLinkedList<class MirBlock> *blockList,
                                            TypedPoolLinkedList<class MirBlock>::Iterator it)
{
    for (auto blockIt = it; blockIt != blockList->end(); ++blockIt)
    {
        MirBlock *block = *blockIt;
        size_t blockId = block->getId();
        TypedPoolLinkedList<MirInstruction> *instructions = block->getInstructions();

        if (!instructions)
            continue;

        for (auto instIt = instructions->begin(); instIt != instructions->end(); ++instIt)
        {
            MirInstruction *inst = *instIt;
            TypedPoolLinkedList<MirOperand> *operands = inst->getOperands();

            if (!operands)
                continue;

            const auto &metadataOperands = inst->getMetadata().m_operandConstraints;

            // Look for defs (writes).
            size_t opIndex = 0;
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt, ++opIndex)
            {
                MirOperand *op = *opIt;

                if (!op->isOfType<MirRegister>())
                    continue;
                if (opIndex >= metadataOperands.size())
                    continue;

                MirRegister *reg = op->get<MirRegister>();
                OperandConstraint constraint = metadataOperands[opIndex];

                if (reg && reg->isVirtual() && (constraint.flags & OperandFlag::Write))
                {
                    m_result.m_def[blockId].insert(reg);
                }
            }

            // Look for uses (reads).
            opIndex = 0;
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt, ++opIndex)
            {
                MirOperand *op = *opIt;

                if (!op->isOfType<MirRegister>())
                    continue;

                if (opIndex >= metadataOperands.size())
                    continue;

                MirRegister *reg = op->get<MirRegister>();
                OperandConstraint constraint = metadataOperands[opIndex];

                if (reg && reg->isVirtual() && (constraint.flags & OperandFlag::Read))
                {
                    // If it is read before being written in this block, it's a use.
                    if (m_result.m_def[blockId].find(reg) == m_result.m_def[blockId].end())
                    {
                        m_result.m_use[blockId].insert(reg);
                    }
                }
            }
        }
    }
}

void LivenessAnalysis::computeGlobalLiveness(TypedPoolLinkedList<class MirBlock> *blockList,
                                             TypedPoolLinkedList<class MirBlock>::Iterator it,
                                             const ControlFlowResult &cfg)
{
    bool changed = true;
    while (changed)
    {
        changed = false;

        // Iterate backwards safely. Assuming `rbegin` and `rend` exist and work as expected.
        for (auto reverseIterator = blockList->rbegin(); reverseIterator != blockList->rend(); ++reverseIterator)
        {
            MirBlock *block = *reverseIterator;
            size_t blockId = block->getId();
            std::unordered_set<MirRegister *> newLiveOut;

            auto succIt = cfg.m_successors.find(block);
            if (succIt != cfg.m_successors.end())
            {
                for (MirBlock *succ : succIt->second)
                {
                    for (MirRegister *reg : m_result.m_liveIn[succ->getId()])
                    {
                        newLiveOut.insert(reg);
                    }
                }
            }

            m_result.m_liveOut[blockId] = newLiveOut;

            std::unordered_set<MirRegister *> newLiveIn = m_result.m_use[blockId];

            for (MirRegister *reg : m_result.m_liveOut[blockId])
            {
                if (m_result.m_def[blockId].find(reg) == m_result.m_def[blockId].end())
                {
                    newLiveIn.insert(reg);
                }
            }

            if (newLiveIn != m_result.m_liveIn[blockId])
            {
                m_result.m_liveIn[blockId] = std::move(newLiveIn);
                changed = true;
            }

            // Manually break *after* we've processed the target iterator 'it'
            // (Assumes `*reverseIterator == *it` is checking block pointer equality)
            if (*reverseIterator == *it)
            {
                break;
            }
        }
    }
}

MirPassIterationPlace LivenessAnalysis::getIterationPlace() const { return MirPassIterationPlace::Block; }
