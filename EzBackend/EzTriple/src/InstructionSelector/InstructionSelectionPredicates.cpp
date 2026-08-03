#include "InstructionSelector/InstructionSelectionPredicates.h"

namespace ISelPreds
{

InstructionSelPred _not(const InstructionSelPred &pred)
{
    return [pred](const SelectionContext &sCtx) -> bool { return !pred(sCtx); };
}

InstructionSelPred _and(const InstructionSelPred &pred1, const InstructionSelPred &pred2)
{
    return [pred1, pred2](const SelectionContext &sCtx) -> bool { return pred1(sCtx) && pred2(sCtx); };
}

InstructionSelPred _or(const InstructionSelPred &pred1, const InstructionSelPred &pred2)
{
    return [pred1, pred2](const SelectionContext &sCtx) -> bool { return pred1(sCtx) || pred2(sCtx); };
}

InstructionSelPred opcode(MirInstructionOpCode opcode)
{
    return [opcode](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        return instr->getOpCode() == opcode;
    };
}

InstructionSelPred operandType(size_t index, MirOperandType operType)
{
    return [index, operType](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();
        return operands.size() > index && operands[index]->getType() == operType;
    };
}

InstructionSelPred operandMirType(size_t index, MirType *type)
{
    return [index, type](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();
        return operands.size() > index && operands[index]->getMirType()->getId() == type->getId();
    };
}

InstructionSelPred operandInt(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::Integer)
            return false;

        return operands[index]->get<MirInteger>()->getValue().getBitSize() == bitWidth;
    };
}

InstructionSelPred operandIntFitsInUnsigned(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::Integer)
            return false;
        return operands[index]->get<MirInteger>()->getValue().fitsIn(bitWidth, false);
    };
}

InstructionSelPred operandIntFitsInSigned(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::Integer)
            return false;
        return operands[index]->get<MirInteger>()->getValue().fitsIn(bitWidth, true);
    };
}

InstructionSelPred operandIntSigned(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::Integer)
            return false;

        const auto &value = operands[index]->get<MirInteger>()->getValue();
        return value.getBitSize() == bitWidth && value.isSigned();
    };
}

InstructionSelPred operandFloat(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::FloatingPoint)
            return false;

        return operands[index]->get<MirFloat>()->getValue().getBitSize() == bitWidth;
    };
}

InstructionSelPred operandFloatFitsIn(size_t index, size_t bitWidth)
{
    return [index, bitWidth](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index || operands[index]->getMirType()->getKind() != MirTypeKind::FloatingPoint)
            return false;

        return operands[index]->get<MirFloat>()->getValue().fitsIn(bitWidth);
    };
}

} // namespace ISelPreds