#ifndef EZMIR_MIR_INSTRUCTION_H
#define EZMIR_MIR_INSTRUCTION_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirOperand.h"
#include "Operand/MirRegisterReference.h"
#include "HelperClasses/IntrusiveLinkedList.h"

#include <vector>

/**
 * Represents a single instruction in the Machine Intermediate Representation (MIR).
 *
 * Implements nodes of an intrusive doubly-linked list within its parent MirBlock.
 * Encapsulates an opcode, operand list, target descriptor (for selected target machine instructions),
 * and metadata providing semantic flags and operand schema.
 */
class MirInstruction
{
  public:
    friend class IntrusiveLinkedList<MirInstruction>;
    friend class MirBlockBuilder;
    friend class MirFunctionBuilder;
    friend class MirInstructionBuilder;

    /**
     * Constructs an instruction record within an owning basic block with opcode, source ref, and operand list.
     */
    explicit MirInstruction(class MirBlock *owner,
                            MirInstructionOpCode opcode,
                            class SourceReference *ref,
                            std::pmr::vector<class MirOperand *> operands);

    /**
     * Checks if this instruction's opcode matches the specified opcode.
     */
    bool hasOpcode(MirInstructionOpCode opcode) const;

    /**
     * Checks whether the instruction has one or more operands.
     */
    bool hasOperands() const;

    /**
     * Checks if this instruction has been lowered to a target machine instruction (opcode TARGET_INST with targetDesc).
     */
    bool isSelected() const;

    /**
     * Checks if this instruction performs signed arithmetic or comparison based on its metadata flags.
     */
    bool isSigned() const;

    /**
     * Returns the mnemonic name of the instruction opcode.
     */
    const char *getOpCodeName() const;

    /**
     * Returns the basic block owning this instruction.
     */
    class MirBlock *getOwner() const;

    /**
     * Returns the preceding instruction in the basic block's intrusive list.
     */
    MirInstruction *getPrev() const;

    /**
     * Returns the subsequent instruction in the basic block's intrusive list.
     */
    MirInstruction *getNext() const;

    /**
     * Returns the static metadata associated with this instruction's opcode.
     */
    const MirInstructionMetadata &getMetadata() const;

    /**
     * Returns the opcode enumeration value of this instruction.
     */
    MirInstructionOpCode getOpCode() const;

    /**
     * Returns the abstraction tier of this instruction (HighLevel, PassInternal, TargetLow).
     */
    MirInstructionTier getTier() const;

    /**
     * Returns the behavioral semantic flags associated with this instruction.
     */
    MirInstructionFlags getFlags() const;

    /**
     * Returns the target machine instruction descriptor, if selected.
     */
    class MirTargetInstructionDesc *getTargetDesc() const;

    /**
     * Retrieves the operand at the specified index, or nullptr if out of bounds.
     */
    MirOperand *getOperand(size_t index) const;

    /**
     * Retrieves the const operand at the specified index, or nullptr if out of bounds.
     */
    const MirOperand *getConstOperand(size_t index) const;

    /**
     * Resolves the operand access flag (Read, Write, ReadWrite) for the operand at index,
     * supporting fixed operands and elastic variadic argument slots.
     */
    MirOperandFlag getOperandFlag(size_t index) const;

    /**
     * Returns the total number of operands attached to this instruction.
     */
    size_t getOperandCount() const;

    /**
     * Returns the source location reference for diagnostics.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Casts and retrieves the const operand at the given index to concrete operand type T.
     */
    template <typename T>
        requires(std::is_const_v<T>)
    const T *getOpAs(size_t index) const
    {
        const MirOperand *op = getConstOperand(index);
        return op ? op->get<T>() : nullptr;
    }

    /**
     * Casts and retrieves the mutable operand at the given index to concrete operand type T.
     */
    template <typename T>
        requires(!std::is_const_v<T>)
    T *getOpAs(size_t index) const
    {
        MirOperand *op = getOperand(index);
        return op ? op->get<T>() : nullptr;
    }

    /**
     * Returns the inmutable reference to the internal operand vector.
     */
    const std::pmr::vector<class MirOperand *> &getOperands() const;

    /**
     * Computes the set of registers defined (written) by this instruction, including explicit and target implicit defs.
     */
    std::vector<MirRegisterRef> getDefinedRegisters() const;

    /**
     * Computes the set of registers used (read) by this instruction, including explicit, memory base, and implicit
     * uses.
     */
    std::vector<MirRegisterRef> getUsedRegisters() const;

    /**
     * Formats the instruction into assembly text format.
     */
    std::string toString() const;

  private:
    /**
     * Updates the opcode of this instruction.
     */
    void setOpcode(MirInstructionOpCode opcode);

    /**
     * Associates a target machine instruction descriptor for lowered instructions.
     */
    void setTargetDesc(MirTargetInstructionDesc *desc);
    
    /**
     * Sets the preceding instruction in the block's intrusive list.
     */
    void setPrev(MirInstruction *prev);

    /**
     * Sets the subsequent instruction in the block's intrusive list.
     */
    void setNext(MirInstruction *next);

  private:
    /**
     * Owning basic block containing this instruction.
     */
    class MirBlock *m_owner;

    /**
     * Intrusive pointer to previous instruction.
     */
    MirInstruction *m_prev{ nullptr };

    /**
     * Intrusive pointer to next instruction.
     */
    MirInstruction *m_next{ nullptr };

    /**
     * Opcode identifier.
     */
    MirInstructionOpCode m_opcode;

    /**
     * Low-level target machine instruction descriptor (null for generic IR).
     */
    class MirTargetInstructionDesc *m_targetDesc;

    /**
     * Source code reference for diagnostics.
     */
    class SourceReference *m_sourceRef;

    /**
     * List of operand pointers allocated in the context memory arena.
     */
    std::pmr::vector<class MirOperand *> m_operands;
};

#endif // EZMIR_MIR_INSTRUCTION_H