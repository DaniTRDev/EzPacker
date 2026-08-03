#ifndef EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H
#define EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H

#include "EzTripleCommon.h"

/**
 * This function is able to look upwards/downwards to check the instruction window of the affected instruction (it).
 * This can be used to apply optimisations when selecting an instruction.
 */
using InstructionSelPred = std::function<bool(std::pmr::list<MirInstruction *> &instrList,
                                              std::pmr::list<MirInstruction *>::iterator it,
                                              MirBuilderContext *ctx)>;

namespace ISelPreds
{
/**
 * Creates a new predicate that returns true if the given predicate returned false.
 */
InstructionSelPred _not(InstructionSelPred pred)
{
    return [pred](std::pmr::list<MirInstruction *> &instrList,
                  std::pmr::list<MirInstruction *>::iterator it,
                  MirBuilderContext *ctx) -> bool { return !pred(instrList, it, ctx); };
}

/**
 * Creates a new predicate that returns true if both, pred1 and pred2, return true.
 */
::InstructionSelPred _and(InstructionSelPred pred1, InstructionSelPred pred2)
{
    return [pred1, pred2](std::pmr::list<MirInstruction *> &instrList,
                          std::pmr::list<MirInstruction *>::iterator it,
                          MirBuilderContext *ctx) -> bool
    { return pred1(instrList, it, ctx) && pred2(instrList, it, ctx); };
}

/**
 * Creates a new predicate that returns true if pred1 or pred2 returns true. If pred1 returns true, pred2 won't be
 * called.
 */
InstructionSelPred _or(InstructionSelPred pred1, ::InstructionSelPred pred2)
{
    return [pred1, pred2](std::pmr::list<MirInstruction *> &instrList,
                          std::pmr::list<MirInstruction *>::iterator it,
                          MirBuilderContext *ctx) -> bool
    { return pred1(instrList, it, ctx) || pred2(instrList, it, ctx); };
}

/**
 * Creates a new predicate that returns true if the instruction opcode matches the one given.
 */
InstructionSelPred opcode(MirInstructionOpCode opcode)
{
    return [opcode](std::pmr::list<MirInstruction *> &instrList,
                    std::pmr::list<MirInstruction *>::iterator it,
                    MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        return instr->getOpCode() == opcode;
    };
}

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's type
 * matches the one given.
 */
InstructionSelPred operandType(size_t index, MirOperandType operType)
{
    return [index, operType](std::pmr::list<MirInstruction *> &instrList,
                             std::pmr::list<MirInstruction *>::iterator it,
                             MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        const auto &operands = instr->getOperands();
        return operands.size() > index && operands[index]->getType() == operType;
    };
}

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's MIR type
 * matches the one given.
 */
InstructionSelPred operandMirType(size_t index, MirType *type)
{
    return [index, type](std::pmr::list<MirInstruction *> &instrList,
                         std::pmr::list<MirInstruction *>::iterator it,
                         MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        const auto &operands = instr->getOperands();
        return operands.size() > index && operands[index]->getMirType()->getId() == type->getId();
    };
}

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's an
 * integer immediate of the given bit-width.
 */
InstructionSelPred operandIntImm(size_t index, size_t bitWidth)
{
    return [index, bitWidth](std::pmr::list<MirInstruction *> &instrList,
                             std::pmr::list<MirInstruction *>::iterator it,
                             MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        const auto &operands = instr->getOperands();

        if (operands.size() > index && operands[index]->getMirType()->getKind() != MirTypeKind::Integer)
            return false;

        return operands[index]->get<MirInteger>()->getValue().getBitSize() == bitWidth;
    };
}

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's an
 * integer immediate of the given bit-width and signed.
 */
InstructionSelPred operandIntImmSigned(size_t index, size_t bitWidth)
{
    return [index, bitWidth](std::pmr::list<MirInstruction *> &instrList,
                             std::pmr::list<MirInstruction *>::iterator it,
                             MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        const auto &operands = instr->getOperands();

        if (operands.size() > index && operands[index]->getMirType()->getKind() != MirTypeKind::FloatingPoint)
            return false;

        const auto &value = operands[index]->get<MirInteger>()->getValue();
        return value.getBitSize() == bitWidth && value.isSigned();
    };
}

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's a
 * floating point immediate of the given bit-width.
 */
InstructionSelPred operandFloatImm(size_t index, size_t bitWidth)
{
    return [index, bitWidth](std::pmr::list<MirInstruction *> &instrList,
                             std::pmr::list<MirInstruction *>::iterator it,
                             MirBuilderContext *ctx) -> bool
    {
        MirInstruction *instr = *it;
        const auto &operands = instr->getOperands();

        if (operands.size() > index && operands[index]->getMirType()->getKind() != MirTypeKind::FloatingPoint)
            return false;

        return operands[index]->get<MirInteger>()->getValue().getBitSize() == bitWidth;
    };
}

}; // namespace ISelPreds

struct InstructionSelectionRule
{
    const char *m_name;        // Important for debug purposes.
    InstructionSelPred m_pred; // If the predicates evaluates to true, the given instruction will be lowered into the
                               // target opcode.
    MirInstructionOpCode m_opcode;          // OpCode this rule was designed for.
    MirTargetInstructionId m_targetInstrId; // Id of the instruction relative to the target.
};

class InstructionSelectionRuleBuilder
{
  public:
    /**
     * Creates the selection rule builder linked to the given context.
     */
    InstructionSelectionRuleBuilder(MirBuilderContext *ctx);

    InstructionSelectionRuleBuilder &begin(const char *name, MirInstruction opcode);

    InstructionSelectionRuleBuilder &selectIf(InstructionSelPred pred);

  private:
    const char *m_ruleName;
    MirBuilderContext *m_ctx;
    MirInstructionOpCode m_opcode;
    MirTargetInstructionId m_targetId;
};

#endif // EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H