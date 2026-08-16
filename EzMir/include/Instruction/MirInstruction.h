#ifndef EZMIR_MIR_INSTRUCTION_H
#define EZMIR_MIR_INSTRUCTION_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirRegisterReference.h"

class MirInstruction
{
  public:
    /**
     * Creates an instruction wrapper around an opcode and its operand list inside an owning block.
     */
    explicit MirInstruction(class MirBlock *owner,
                            MirInstructionOpCode opcode,
                            class SourceReference *ref,
                            std::pmr::vector<class MirOperand *> operands);

    /**
     * Returns `true` when the instruction currently stores at least one
     * operand in its operand list.
     */
    bool hasOperands() const;

    /**
     * Returns true if this instruction is selected: m_opcode == MirInstructionOpCode::TARGET_INST AND m_targetInstDesc
     * != nullptr.
     */
    bool isSelected() const;

    /**
     * Returns `true` when the instruction's opcode is marked as signed in its metadata flags.
     */
    bool isSigned() const;

    /**
     * Returns the name of the opcode (using metadata).
     */
    const char *getOpCodeName() const;

    /**
     * Returns the owner block of this instruction.
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
     * Returns the tier of the instruction.
     */
    MirInstructionTier getTier() const;

    /**
     * Returns the instruction flags from the opcode metadata.
     */
    MirInstructionFlags getFlags() const;

    /**
     * Returns the target description of the instruction. Will only contain a valid value after instruction selection
     * pass.
     */
    class MirTargetInstructionDesc *getTargetDesc() const;

    /**
     * Returns the operand flag for the given operand distinguishing between a high level mir instruction and a target
     * instruction.
     */
    MirOperandFlag getOperandFlag(size_t index) const;

    /**
     * Returns the source reference of this instruction.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Adds an operand to the instruction. This invalidates cached defined and used registers.
     */
    void addOperand(const MirOperand *operand);

    /**
     * Invalidates cached uses and defs by setting the booleans to false.
     */
    void invalidateCachedUsedAndDefs();

    /**
     * Sets or switched the opcode of the instruction. This invalidates cached defined and used registers.
     */
    void setOpcode(MirInstructionOpCode opcode);

    /**
     * Sets the target descriptor for the instruction. This invalidates cached defined and used registers.
     */
    void setTargetDesc(MirTargetInstructionDesc *desc);

    /**
     * Replaces the operands of this instruction with the ones given. Also invalidates cached defined and used
     * registers.
     *
     * Caller must ensure that the resource that allocated operands is alive when using this object.
     */
    void setOperands(const std::pmr::vector<class MirOperand *> &operands);

    /**
     * Returns the immutable operand slice for this instruction.
     */
    const std::pmr::vector<class MirOperand *> &getOperands() const;

    /**
     * Returns the mutable operand slice for this instruction. This will invalidate cached used and defined registers.
     */
    std::pmr::vector<class MirOperand *> &getOperands();

    /**
     * Returns the registers defined (written) by this instruction.
     */
    const std::pmr::vector<MirRegisterRef> &getDefinedRegisters();

    /**
     * Returns the registers used (read) by this instruction.
     */
    const std::pmr::vector<MirRegisterRef> &getUsedRegisters();

    /**
     * Returns a string representation of the instruction in assembly format.
     */
    std::string toString() const;

  private:
    bool m_cachedDefinedRegisters;
    bool m_cachedUsedRegisters;
    class MirBlock *m_owner;
    MirInstructionOpCode m_opcode;
    class MirTargetInstructionDesc *m_targetDesc;
    class SourceReference *m_sourceRef;
    std::pmr::vector<class MirOperand *> m_operands;
    std::pmr::vector<MirRegisterRef> m_definedRegisters;
    std::pmr::vector<MirRegisterRef> m_usedRegisters;
};

#endif // EZMIR_MIR_INSTRUCTION_H
