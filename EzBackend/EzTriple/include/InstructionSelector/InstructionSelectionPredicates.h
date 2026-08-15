#ifndef EZPACKER_INSTRUCTIONSELECTIONPREDICATES_H
#define EZPACKER_INSTRUCTIONSELECTIONPREDICATES_H

#include "EzTripleCommon.h"
#include "InstructionSelectionRuleBuilder.h"

namespace ISelPreds
{
/**
 * Creates a new predicate that returns true if the given predicate returned false.
 */
extern InstructionSelPred _not(const InstructionSelPred &pred);

/**
 * Creates a new predicate that returns true if both, pred1 and pred2, return true.
 */
extern InstructionSelPred _and(const InstructionSelPred &pred1, const InstructionSelPred &pred2);

/**
 * Creates a new predicate that returns true if pred1 or pred2 returns true. If pred1 returns true, pred2 won't be
 * called.
 */
extern InstructionSelPred _or(const InstructionSelPred &pred1, const InstructionSelPred &pred2);

/**
 * Creates a new predicate that returns true if the instruction opcode matches the one given.
 */
extern InstructionSelPred opcode(MirInstructionOpCode opcode);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's type
 * matches the one given.
 */
extern InstructionSelPred operandType(size_t index, MirOperandType operType);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's MIR type
 * matches the one given.
 */
extern InstructionSelPred operandMirType(size_t index, MirType *type);

/**
 * Creates a new predicate that returns true if instruction has an operand at the given index, and if it's MIR type kind
 * matches the one given..
 */
extern InstructionSelPred operandMirTypeKind(size_t index, MirTypeKind kind);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's an
 * integer immediate of the given bit-width.
 */
extern InstructionSelPred operandInt(size_t index, size_t bitWidth);

/**
 * Returns true if there's an operand at the given index, if it is an integer and if it fits in an unsigned container of
 * bitWidth size.
 * @param index
 * @param bitWidth
 * @return
 */
extern InstructionSelPred operandIntFitsInUnsigned(size_t index, size_t bitWidth);

/**
 * Returns true if there's an operand at the given index, if it is an integer and if it fits in an signed container of
 * bitWidth size.
 * @param index
 * @param bitWidth
 * @return
 */
extern InstructionSelPred operandIntFitsInSigned(size_t index, size_t bitWidth);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's an
 * integer immediate of the given bit-width and signed.
 */
extern InstructionSelPred operandIntSigned(size_t index, size_t bitWidth);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, and if it's a
 * floating point immediate of the given bit-width.
 */
extern InstructionSelPred operandFloat(size_t index, size_t bitWidth);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, if it's a
 * floating point immediate and if it fits in a container of bitWidth size. This will use IEEE-754.
 */
extern InstructionSelPred operandFloatFitsIn(size_t index, size_t bitWidth);

}; // namespace ISelPreds

#endif // EZPACKER_INSTRUCTIONSELECTIONPREDICATES_H
