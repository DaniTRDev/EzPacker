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
#include "Operand/MirOperand.h"

class MirInstruction
{
  public:
    /**
     * Creates an instruction wrapper around an opcode and its operand slice.
     *
     * @param opcode   Opcode describing the operation performed.
     * @param operands Arena-managed operand slice initially associated with the
     *                 instruction. Emitters append to this slice later.
     */
    MirInstruction(MirInstructionOpCode opcode, TypedPoolSlice<MirOperand> *operands);

    /**
     * Returns `true` when the instruction currently stores at least one
     * operand in its operand slice.
     */
    bool hasOperands() const;

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
    TypedPoolSlice<MirOperand> *getOperands();

    /**
     * Returns the instruction flags from the opcode metadata.
     */
    uint32_t getFlags() const;

  private:
    MirInstructionOpCode m_opcode;
    TypedPoolSlice<MirOperand> *m_operands;
};

#endif // EZPACKER_MIRINSTRUCTION_H
