#ifndef EZMIR_MIR_INSTRUCTION_H
#define EZMIR_MIR_INSTRUCTION_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirOperand.h"
#include "Operand/MirRegisterReference.h"

#include <vector>

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
     * Returns true if the instruction's opcode matches the given opcode.
     */
    bool hasOpcode(MirInstructionOpCode opcode) const;

    /**
     * Returns true when the instruction currently stores at least one operand.
     */
    bool hasOperands() const;

    /**
     * Returns true if this instruction has been lowered to a target machine instruction.
     */
    bool isSelected() const;

    /**
     * Returns true when the instruction's opcode is marked as signed in its metadata flags.
     */
    bool isSigned() const;

    /**
     * Returns the name of the opcode.
     */
    const char *getOpCodeName() const;

    /**
     * Returns the owner block of this instruction.
     */
    class MirBlock *getOwner() const;

    /**
     * Returns the previous instruction.
     */
    MirInstruction *getPrev() const;

    /**
     * Returns the next instruction.
     */
    MirInstruction *getNext() const;

    /**
     * Returns the metadata entry associated with this instruction's opcode.
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
     * Returns the target instruction descriptor if lowered.
     */
    class MirTargetInstructionDesc *getTargetDesc() const;

    /**
     * Returns a pointer to the operand at the given index, or nullptr if out of bounds.
     */
    MirOperand *getOperand(size_t index) const;

    /**
     * Returns a const pointer to the operand at the given index, or nullptr if out of bounds.
     */
    const MirOperand *getConstOperand(size_t index) const;

    /**
     * Returns the operand flag for the given index.
     */
    MirOperandFlag getOperandFlag(size_t index) const;

    /**
     * Returns the number of operands.
     */
    size_t getOperandCount() const;

    /**
     * Returns the source reference of this instruction.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns a const pointer to the typed operand at the given index.
     */
    template <typename T>
        requires(std::is_const_v<T>)
    const T *getOpAs(size_t index) const
    {
        const MirOperand *op = getConstOperand(index);
        return op ? op->get<T>() : nullptr;
    }

    /**
     * Returns a pointer to the typed operand at the given index.
     */
    template <typename T>
        requires(!std::is_const_v<T>)
    T *getOpAs(size_t index) const
    {
        MirOperand *op = getOperand(index);
        return op ? op->get<T>() : nullptr;
    }

    /**
     * Adds an operand to the instruction.
     */
    void addOperand(MirOperand *operand);

    /**
     * Sets or switches the opcode of the instruction.
     */
    void setOpcode(MirInstructionOpCode opcode);

    /**
     * Sets the target descriptor for the lowered instruction.
     */
    void setTargetDesc(MirTargetInstructionDesc *desc);

    /**
     * Replaces the operands of this instruction.
     */
    void setOperands(const std::pmr::vector<class MirOperand *> &operands);

    /**
     * Sets the previous instruction.
     */
    void setPrev(MirInstruction *prev);

    /**
     * Sets the next instruction.
     */
    void setNext(MirInstruction *next);

    /**
     * Returns the immutable operand slice.
     */
    const std::pmr::vector<class MirOperand *> &getOperands() const;

    /**
     * Returns the mutable operand slice.
     */
    std::pmr::vector<class MirOperand *> &getOperands();

    /**
     * Computes and returns the registers defined (written) by this instruction on-the-fly.
     */
    std::vector<MirRegisterRef> getDefinedRegisters() const;

    /**
     * Computes and returns the registers used (read) by this instruction on-the-fly.
     */
    std::vector<MirRegisterRef> getUsedRegisters() const;

    /**
     * Returns a string representation of the instruction in assembly format.
     */
    std::string toString() const;

  private:
    class MirBlock *m_owner;
    MirInstruction *m_prev{ nullptr };
    MirInstruction *m_next{ nullptr };
    MirInstructionOpCode m_opcode;
    class MirTargetInstructionDesc *m_targetDesc;
    class SourceReference *m_sourceRef;
    std::pmr::vector<class MirOperand *> m_operands;
};

#endif // EZMIR_MIR_INSTRUCTION_H