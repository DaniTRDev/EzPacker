#include "TargetInstructionSelector/InstructionSelectionTable.h"

bool SelectionRule::matches(MirInstruction *instr) const
{
    auto operands = instr->getOperands();

    // If both constraint arrays are empty, it's an unconditional match.
    if (m_operandKinds.empty() && m_operandSizes.empty())
        return true;

    // Check Operand Kinds (if the array is not empty)
    if (!m_operandKinds.empty())
    {
        if (m_operandKinds.size() != operands->m_numElems)
            return false;

        size_t i = 0;
        for (auto it = operands->begin(); it != operands->end(); ++it, ++i)
        {
            MirOperand *op = *it;
            ExpectedOperandType expectedKind = m_operandKinds[i];

            if ((expectedKind & ExpectedOperandType::Register) && op->isOfType<MirRegister>())
                continue;
            if ((expectedKind & ExpectedOperandType::Integer) && op->isOfType<MirInteger>())
                continue;
            if ((expectedKind & ExpectedOperandType::Double) && op->isOfType<MirDouble>())
                continue;
            if ((expectedKind & ExpectedOperandType::Memory) && op->isOfType<MirMemory>())
                continue;
            if ((expectedKind & ExpectedOperandType::FrameIndex) && op->isOfType<MirFrameIndex>())
                continue;
            if ((expectedKind & ExpectedOperandType::Reference) && op->isOfType<MirReference>())
                continue;

            return false; // No match
        }
    }

    // Check Operand Sizes (if the array is not empty)
    if (!m_operandSizes.empty())
    {
        if (m_operandSizes.size() != operands->m_numElems)
            return false;

        size_t i = 0;
        for (auto it = operands->begin(); it != operands->end(); ++it, ++i)
        {
            MirOperand *op = *it;
            if (op->getSizeInBytes() != m_operandSizes[i])
                return false;
        }
    }

    return true; // Passed all specified constraints
}

void InstructionSelectionTable::addRule(MirInstructionOpCode opcode, const SelectionRule &rule)
{
    m_rules[opcode].push_back(rule);
}

MirTargetInstructionId InstructionSelectionTable::select(MirInstruction *instr) const
{
    auto it = m_rules.find(instr->getOpCode());
    if (it == m_rules.end())
        return TARGET_INSTR_SELECT_NONE;

    // Iterate through rules in the order they were added.
    // The first one that matches wins.
    for (const SelectionRule &rule : it->second)
    {
        if (rule.matches(instr))
        {
            return rule.m_hardwareId;
        }
    }

    return TARGET_INSTR_SELECT_NONE;
}
