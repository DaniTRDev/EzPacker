#include "TargetLegalizer/LegalizerActionList.h"

uint8_t LegalizerActionList::getOperandAction(MirInstructionOpCode opcode, MirId typeId) const
{
    return m_operandActions[opcode].count(typeId) > 0 ? m_operandActions[opcode].at(typeId)
                                                      : Action_None;
}

void LegalizerActionList::setOperandAction(MirInstructionOpCode opcode, MirId typeId, uint8_t actionType)
{
    m_operandActions[opcode].insert({ typeId, actionType });
}

void LegalizerActionList::setOperandActionForClass(MirInstructionCategory instrCategory,
                                                   MirId typeId,
                                                   uint8_t actionType)
{
    for (size_t i = 0; i < static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT); i++)
    {
        const MirInstructionMetadata &meta = getMeta(static_cast<MirInstructionOpCode>(i));
        if (meta.m_category == instrCategory)
        {
            setOperandAction(static_cast<MirInstructionOpCode>(i), typeId, actionType);
        }
    }
}