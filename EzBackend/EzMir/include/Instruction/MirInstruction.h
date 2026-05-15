/**
 * @file MirInstruction.h
 * @brief A single MIR instruction: opcode + operand list.
 *
 * `MirInstruction` is the executable atom stored inside a `MirBlock`. It owns
 * no heap memory itself; instead, it points at an arena-managed operand slice
 * created by `MirEmitterContext`. The opcode determines how many operands are
 * expected and which semantic flags apply, via `MirInstructionMetadata` from
 * the compile-time instruction catalogue.
 */
#ifndef EZPACKER_MIRINSTRUCTION_H
#define EZPACKER_MIRINSTRUCTION_H

#include "EzMirCommon.h"
#include "MirInstructionDefs.h"
#include "Operand/MirOperands.h"

class MirInstruction
{
  public:
    /**
     * Creates an instruction wrapper around an opcode and its operand slice.
     *
     * @param opcode   Opcode describing the operation performed.
     * @param operands Arena-managed operand list initially associated with the
     *                 instruction. Emitters append to this list later.
     */
    explicit MirInstruction(MirInstructionOpCode opcode, TypedPoolLinkedList<MirOperand> *operands);

    /**
     * Creates an instruction wrapper around an opcode and its operand slice, with a lowered opcode for instruction
     * selectors to store the result of selection.
     * @param opcode
     * @param operands
     */
    explicit MirInstruction(size_t opcode, TypedPoolLinkedList<MirOperand> *operands);

    /**
     * Returns `true` when the instruction currently stores at least one
     * operand in its operand slice.
     */
    bool hasOperands() const;

    /**
     * Returns `true` when the instruction's opcode is marked as signed in its metadata flags.
     * @return
     */
    bool isSigned() const;

    /**
     * Returns the linear equivalent of this instruction. If this instruction does not have any linear equivalent,
     * a pair of INVALID, INVALID is returned.
     * @return
     */
    const MirInstructionLinearEquivalent &getLinearEquivalent() const;

    /**
     * Returns the metadata entry associated with this instruction's opcode.
     *
     * The metadata contains the printable opcode name, expected operand count,
     * and instruction flags.
     */
    const MirInstructionMetadata &getMetadata() const;

    /**
     * Returns this instruction's opcode.
     */
    MirInstructionOpCode getOpCode() const;

    /**
     * Returns the mutable operand slice for this instruction.
     */
    TypedPoolLinkedList<MirOperand> *getOperands() const;

    /**
     * Returns the instruction flags from the opcode metadata.
     */
    MirInstructionFlags getFlags() const;

    /**
     * Returns a string representation of the instruction in assembly format.
     * @return std::string
     */
    std::string toString() const;

  private:
    MirInstructionOpCode m_opcode;
    TypedPoolLinkedList<MirOperand> *m_operands;
};

#endif // EZPACKER_MIRINSTRUCTION_H
