#include "InstructionSelector/InstructionSelectionPredicates.h"

namespace ISelPreds
{

InstructionSelPred _not(const InstructionSelPred &pred)
{
    return [pred](const SelectionContext &sCtx) -> bool { return !pred(sCtx); };
}

InstructionSelPred codeModel(CodeModel expected)
{
    return [expected](const SelectionContext &ctx) -> bool
    { return ctx.m_targetBinaryDesc->getCodeModel() == expected; };
}

InstructionSelPred opcode(MirInstructionOpCode opcode)
{
    return [opcode](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        return instr->getOpCode() == opcode;
    };
}

InstructionSelPred operandIsGlobalRef(size_t index)
{
    return [index](const SelectionContext &ctx) -> bool
    {
        MirInstruction *instr = *ctx.m_it;
        const auto &operands = instr->getOperands();

        if (operands.size() <= index)
            return false;

        MirOperand *op = operands[index];
        MirReference *ref = op->get<MirReference>();

        return ref && (ref->isGlobalVar() || ref->isFunction() || ref->isGlobalArrayElem());
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

InstructionSelPred operandMirTypeKind(size_t index, MirTypeKind kind)
{
    return [index, kind](const SelectionContext &sCtx) -> bool
    {
        MirInstruction *instr = *sCtx.m_it;
        const auto &operands = instr->getOperands();
        return operands.size() > index && operands[index]->getMirType()->getKind() == kind;
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