#include "TargetLegalizer/LegalizerActionList.h"

TargetLegalizerActionType LegalizerActionList::getOperandAction(MirInstructionOpCode opcode, MirId typeId) const
{
    return m_operandActions[opcode].count(typeId) > 0 ? m_operandActions[opcode].at(typeId) : Action_None;
}

void LegalizerActionList::setOperandAction(MirInstructionOpCode opcode,
                                           MirId typeId,
                                           TargetLegalizerActionType actionType)
{
    m_operandActions[opcode][typeId] = actionType;
}

void LegalizerActionList::setOperandAction(MirInstructionOpCode opcode,
                                           MirType *type,
                                           TargetLegalizerActionType actionType)
{
    setOperandAction(opcode, type->getId(), actionType);
}

void LegalizerActionList::setOperandActionForClass(MirInstructionCategory instrCategory,
                                                   MirId typeId,
                                                   TargetLegalizerActionType actionType)
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

void LegalizerActionList::setOperandActionForClass(MirInstructionCategory instrCategory,
                                                   MirType *type,
                                                   TargetLegalizerActionType actionType)
{
    setOperandActionForClass(instrCategory, type->getId(), actionType);
}
