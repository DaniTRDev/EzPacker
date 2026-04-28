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
    explicit MirInstruction(MirInstructionOpCode opcode, TypedPoolSlice<MirOperand> *operands);

    /**
     * Creates an instruction wrapper around an opcode and its operand slice, with a lowered opcode for instruction
     * selectors to store the result of selection.
     * @param opcode
     * @param operands
     */
    explicit MirInstruction(size_t opcode, TypedPoolSlice<MirOperand> *operands);

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
     * Returns the lowered opcode stored by instruction selectors after selection.
     * @return
     */
    size_t getLoweredOpCode() const;

    /**
     * Returns the mutable operand slice for this instruction.
     */
    TypedPoolSlice<MirOperand> *getOperands() const;

    /**
     * Returns the instruction flags from the opcode metadata.
     */
    uint32_t getFlags() const;

    /**
     * Sets the lowered opcode for instruction selectors to store the result of selection.
     * @param loweredOpCode
     */
    void setLoweredOpCode(size_t loweredOpCode);

    /**
     * Returns a string representation of the instruction in assembly format.
     * @return std::string
     */
    std::string toString() const;

  private:
    MirInstructionOpCode m_opcode;
    size_t m_loweredOpCode; // For instruction selectors to store the lowered opcode after selection
    TypedPoolSlice<MirOperand> *m_operands;
};

#endif // EZPACKER_MIRINSTRUCTION_H
