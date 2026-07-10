/**
 * @file MirInstruction.h
 * @brief A single MIR instruction: opcode + operand list.
 *
 * `MirInstruction` is the executable atom stored inside a `MirBlock`. It owns
 * no heap memory itself; instead, it points at an arena-managed operand vector
 * created by `MirBuilderContext`. The opcode determines how many operands are
 * expected and which semantic flags apply, via `MirInstructionMetadata` from
 * the compile-time instruction catalogue.
 */
#ifndef EZPACKER_MIRINSTRUCTION_H
#define EZPACKER_MIRINSTRUCTION_H

#include "EzMirCommon.h"
#include "MirInstructionDefs.h"
#include "Operand/MirOperands.h"

/**
 * Type used to abstract away details about the selected opcode of a mir instruction (happens in instruction selector
 * pass).
 */
using MirTargetInstructionId = uint32_t;
constexpr MirTargetInstructionId TARGET_INSTR_SELECT_NONE = 0;

class MirInstruction
{
  public:
    /**
     * Creates an instruction wrapper around an opcode and its operand list inside an owning block.
     *
     * @param owner
     * @param opcode   Opcode describing the operation performed.
     * @param ref      Source reference of where this instruction was originated.
     * @param operands Arena-managed operand list initially associated with the
     *                 instruction. Emitters append to this list later.
     */
    explicit MirInstruction(class MirBlock *owner,
                            MirInstructionOpCode opcode,
                            SourceReference *ref,
                            std::pmr::vector<MirOperand *> operands);

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
     * Returns the owner block of this instruction.
     * @return
     */
    class MirBlock *getOwner();

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
     * Returns the instruction flags from the opcode metadata.
     */
    MirInstructionFlags getFlags() const;

    /**
     * Returns the targetId of the instruction. Will only contain a valid value after instruction selection pass.
     * @return
     */
    MirTargetInstructionId getTargetId() const;

    /**
     * Returns the source reference of this instruction.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Adds an operand to the instruction.
     * @param operand
     */
    void addOperand(const MirOperand *operand);

    /**
     * Sets or switched the opcode of the instruction.
     * @param opcode
     */
    void setOpcode(MirInstructionOpCode opcode);

    /**
     * Sets the target instruction ID.
     * @param id
     */
    void setTargetId(MirTargetInstructionId id);

    /**
     * Returns the immutable operand slice for this instruction.
     */
    const std::pmr::vector<MirOperand *> &getOperands() const;

    /**
     * Returns the mutable operand slice for this instruction.
     */
    std::pmr::vector<MirOperand *> &getOperands();

    /**
     * Returns a string representation of the instruction in assembly format.
     * @return std::string
     */
    std::string toString() const;

  private:
    class MirBlock *m_owner;
    MirInstructionOpCode m_opcode;
    MirTargetInstructionId m_targetId;
    SourceReference *m_sourceRef;
    std::pmr::vector<MirOperand *> m_operands;
};

#endif // EZPACKER_MIRINSTRUCTION_H
