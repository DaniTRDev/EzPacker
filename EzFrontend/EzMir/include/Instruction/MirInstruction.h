/**
 * @file MirInstruction.h
 * @brief A single MIR instruction: opcode + operand list.
 *
 * Each MirInstruction carries an opcode (MirInstructionOpCode), a linked
 * list of MirOperand values, and the associated metadata (operand count,
 * flags such as IsTerminator, ReadsCPUFlags, etc.).  Instructions live
 * inside a MirBlock and are allocated from an arena pool.
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
     * Creates the instruction with the given opcode and operand list.
     * @param opcode   The operation this instruction performs.
     * @param operands Initially-empty slice that will hold operands.
     */
    MirInstruction(MirInstructionOpCode opcode, TypedPoolSlice<MirOperand> *operands);

    /**
     * Returns true if this instruction contains at least 1 operand.
     * @return bool
     */
    bool hasOperands() const;

    /**
     * Returns this instruction's metadata.
     * @return const MirInstructionMetadata &
     */
    const MirInstructionMetadata &getMetadata() const;

    /**
     * Returns the instruction's operation code.
     * @return MirInstructionOpCode
     */
    MirInstructionOpCode getOpCode() const;

    /**
     * Returns the operands of this instruction.
     * @return TypedPoolSlice<MirOperand> *
     */
    TypedPoolSlice<MirOperand> *getOperands();

    /**
     * Returns the flags of this instruction. Retrieved from metadata.
     * @return uint32_t
     */
    uint32_t getFlags() const;

  private:
    MirInstructionOpCode m_opcode;
    TypedPoolSlice<MirOperand> *m_operands;
};

#endif // EZPACKER_MIRINSTRUCTION_H
