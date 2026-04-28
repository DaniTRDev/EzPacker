#include "MirPass/Passes/LivenessAnalysis.h"
#include "MirPass/MirPassManager.h"

LivenessAnalysis::LivenessAnalysis(MirEmitter *emitter) : m_emitter(emitter) {}

bool LivenessAnalysis::run(MirFunction *func, MirPassManager *pm)
{
    m_result.m_liveIn.clear();
    m_result.m_liveOut.clear();
    m_result.m_def.clear();
    m_result.m_use.clear();

    auto &cfgPass = pm->getAnalysis<CodeFlowAnalysis>(func, m_emitter);
    const ControlFlowResult &cfg = cfgPass.getResult();

    computeLocalLiveness(func);
    computeGlobalLiveness(func, cfg);

    return true;
}

const LivenessResult &LivenessAnalysis::getResult() const { return m_result; }

void LivenessAnalysis::computeLocalLiveness(MirFunction *func)
{
    TypedPoolSlice<MirBlock> *blocks = func->getBlocks();
    if (!blocks)
        return;

    for (auto blockIt = blocks->begin(); blockIt != blocks->end(); ++blockIt)
    {
        MirBlock *block = *blockIt;
        TypedPoolSlice<MirInstruction> *instructions = block->getInstructions();

        if (!instructions)
            continue;

        for (auto instIt = instructions->begin(); instIt != instructions->end(); ++instIt)
        {
            MirInstruction *inst = *instIt;
            TypedPoolSlice<MirOperand> *operands = inst->getOperands();

            if (!operands)
                continue;

            // Look for uses (reads).
            size_t opIndex = 0;
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt, ++opIndex)
            {
                MirOperand *op = *opIt;
                MirRegister *reg = op->getRegister();
                OperandConstraint constraint = inst->getMetadata().m_operands[opIndex];

                bool isReadOperand = constraint.flags & OperandFlag::Read;

                if (reg && reg->m_virtual && isReadOperand)
                {
                    // If it's defined, insert it in the use.
                    if (m_result.m_def[block].find(*reg) == m_result.m_def[block].end())
                    {
                        m_result.m_use[block].insert(*reg);
                    }
                }
            }

            // Look for defs (writes).
            opIndex = 0;
            for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt, ++opIndex)
            {
                MirOperand *op = *opIt;
                MirRegister *reg = op->getRegister();
                OperandConstraint constraint = inst->getMetadata().m_operands[opIndex];

                bool isWriteOperand = constraint.flags & OperandFlag::Write;
                if (reg && reg->m_virtual && isWriteOperand)
                {
                    m_result.m_def[block].insert(*reg);
                }
            }
        }
    }
}

void LivenessAnalysis::computeGlobalLiveness(MirFunction *func, const ControlFlowResult &cfg)
{
    TypedPoolSlice<MirBlock> *blocks = func->getBlocks();
    if (!blocks || blocks->m_numElems == 0)
        return;

    bool changed = true;

    while (changed)
    {
        changed = false;

        for (auto it = blocks->rbegin(); it != blocks->rend(); ++it)
        {
            MirBlock *block = *it;
            std::unordered_set<MirRegister> newLiveOut;

            auto succIt = cfg.m_successors.find(block);
            if (succIt != cfg.m_successors.end())
            {
                for (MirBlock *succ : succIt->second)
                {
                    for (const MirRegister &reg : m_result.m_liveIn[succ])
                    {
                        newLiveOut.insert(reg);
                    }
                }
            }

            m_result.m_liveOut[block] = newLiveOut;

            std::unordered_set<MirRegister> newLiveIn = m_result.m_use[block];

            for (const MirRegister &reg : m_result.m_liveOut[block])
            {
                if (m_result.m_def[block].find(reg) == m_result.m_def[block].end())
                {
                    newLiveIn.insert(reg);
                }
            }

            if (newLiveIn != m_result.m_liveIn[block])
            {
                m_result.m_liveIn[block] = std::move(newLiveIn);
                changed = true;
            }
        }
    }
}