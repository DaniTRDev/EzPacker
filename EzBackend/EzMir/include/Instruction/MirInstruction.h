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
using MirTargetInstructionId = MirId;

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
     * Returns the name of the opcode (using metadata).
     * @return
     */
    const char *getOpCodeName() const;

    /**
     * Returns the owner block of this instruction.
     * @return
     */
    class MirBlock *getOwner();

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
     * Adds an operand to the instruction. This will set m_cachedDefinedRegisters and m_cachedUsedRegisters to false.
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
     * Invalidates cached uses and defs by setting the booleans to false.
     */
    void invalidateCachedUsedAndDefs();

    /**
     * Returns the immutable operand slice for this instruction.
     */
    const std::pmr::vector<MirOperand *> &getOperands() const;

    /**
     * Returns the mutable operand slice for this instruction. This will set m_cachedDefinedRegisters and
     * m_cachedUsedRegisters to false.
     */
    std::pmr::vector<MirOperand *> &getOperands();

    /**
     * Returns the registers defined (written) by this instruction.
     */
    const std::pmr::vector<RegisterRef> &getDefinedRegisters();

    /**
     * Returns the registers used (read) by this instruction.
     */
    const std::pmr::vector<RegisterRef> &getUsedRegisters();

    /**
     * Returns a string representation of the instruction in assembly format.
     * @return std::string
     */
    std::string toString() const;

  private:
    bool m_cachedDefinedRegisters;
    bool m_cachedUsedRegisters;
    class MirBlock *m_owner;
    MirInstructionOpCode m_opcode;
    MirTargetInstructionId m_targetId;
    SourceReference *m_sourceRef;
    std::pmr::vector<MirOperand *> m_operands;
    std::pmr::vector<RegisterRef> m_definedRegisters;
    std::pmr::vector<RegisterRef> m_usedRegisters;
};

#endif // EZPACKER_MIRINSTRUCTION_H
