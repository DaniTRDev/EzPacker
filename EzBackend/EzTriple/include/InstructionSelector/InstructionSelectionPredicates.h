#ifndef EZPACKER_INSTRUCTIONSELECTIONPREDICATES_H
#define EZPACKER_INSTRUCTIONSELECTIONPREDICATES_H

#include "EzTripleCommon.h"
#include "InstructionSelectionRuleBuilder.h"
#include "Descriptors/TargetBinaryDesc.h"

namespace ISelPreds
{
/**
 * Creates a new predicate that returns true if the given predicate returned false.
 */
extern InstructionSelPred _not(const InstructionSelPred &pred);

/**
 * Creates a new predicate that returns true if all provided predicates evaluate to true (short-circuiting).
 * Supports chaining 2 or more predicates directly: _and(p1, p2, p3, ...)
 */
template <typename... Preds>
    requires(sizeof...(Preds) >= 2 && (std::is_convertible_v<Preds, InstructionSelPred> && ...))
inline InstructionSelPred _and(Preds &&...preds)
{
    return [preds = std::make_tuple(std::forward<Preds>(preds)...)](const SelectionContext &ctx) -> bool
    { return std::apply([&ctx](const auto &...p) { return (p(ctx) && ...); }, preds); };
}

/**
 * Creates a new predicate that returns true if at least one of the provided predicates evaluates to true
 * (short-circuiting). Supports chaining 2 or more predicates directly: _or(p1, p2, p3, ...)
 */
template <typename... Preds>
    requires(sizeof...(Preds) >= 2 && (std::is_convertible_v<Preds, InstructionSelPred> && ...))
inline InstructionSelPred _or(Preds &&...preds)
{
    return [preds = std::make_tuple(std::forward<Preds>(preds)...)](const SelectionContext &ctx) -> bool
    { return std::apply([&ctx](const auto &...p) { return (p(ctx) || ...); }, preds); };
}

/**
 * Creates a new predicate that returns true if the code model of the target binary descriptor matches the one given.
 */
extern InstructionSelPred codeModel(CodeModel expected);

/**
 * Creates a new predicate that returns true if the instruction opcode matches the one given.
 */
extern InstructionSelPred opcode(MirInstructionOpCode opcode);

/**
 * Creates a new predicate that returns true if the instruction has an operand at the given index, if it's a reference
 * and if it's a global reference (GlobalVar, Function, GlobalArray).
 */
extern InstructionSelPred operandIsGlobalRef(size_t index);

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
