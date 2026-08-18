#ifndef EZPACKER_MIR_INSTRUCTION_BUILDER_H
#define EZPACKER_MIR_INSTRUCTION_BUILDER_H

#include "EzMirCommon.h"
#include "MirInstructionSet.h"
#include "Builder/MirBuilder.h"

enum class InsertionType : uint8_t
{
    InsertAfter,  // After a point. Needs to specify an iterator.
    InsertBefore, // Before a point. Needs to specify an iterator.
    Append // Grabs the last instruction of the owner and pushes right after it. This type DOES not use m_iterator and
           // can be left NULL.
};

/**
 * Structure that contains information about where the instruction produced by a builder should be stored.
 */
struct MirInstructionInsertionPoint
{
    InsertionType m_type;
    class MirBlock *m_block;
    std::pmr::list<class MirInstruction *>::iterator m_iterator{};
};

class MirInstructionBuilder : public MirBuilder<class MirInstruction>
{
  public:
    /**
     * Creates the builder with the given ctx, insertion point and opcode.
     */
    MirInstructionBuilder(class MirBuilderContext *ctx, MirInstructionInsertionPoint insertionPoint);

    /**
     * Creates an instruction builder linked to a block at a specic position, the insertion order can be also set.
     */
    MirInstructionBuilder(class MirBuilderContext *ctx,
                          class MirBlock *block,
                          InsertionType type,
                          std::pmr::list<MirInstruction *>::iterator it = {});

    /**
     * Builds an instruction with the given opcode and inserts it with the insert point information. Given operand's
     * initializer list' elements will be COPIED into the list of operands of the instruction.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::initializer_list<class MirOperand *> &operands = {});

    /**
     * Builds an instruction with the given opcode, operands and inserts it with the insert point information. Given
     * operand's vector' elements will be COPIED into the list of operands of the instruction.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::vector<class MirOperand *> &operands = {});

    /**
     * Builds an instruction with the given opcode, operands and inserts it with the insert point information. Given
     * operand's initializer list' elements will be COPIED into the list of operands of the instruction.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::pmr::vector<class MirOperand *> &operands);

    /**
     * Builds a target instruction with the given target descriptor, scr ref and operands. The opcode of this
     * instruction is set to TARGET_INST. The targetId of the instruction is set to the one given. Given operand's
     * initializer list' elements will be COPIED into the list of operands of the instruction.
     */
    MirInstruction *buildTarget(class MirTargetInstructionDesc *targetDesc,
                                class SourceReference *srcRef,
                                std::initializer_list<class MirOperand *> operands);

    /**
     * Overload of the '<<' operator that allows pushing operands easily in the FUTURE instruction.
     */
    MirInstructionBuilder &operator<<(class MirOperand *operand);

// Define the macro to generate a method for each instruction. This one makes possible attaching a source ref.
#define INSTRUCTION(NAME, tier, category, ops, flags)                                                                  \
    template <typename... OperandTypes>                                                                                \
    MirInstruction *NAME(class SourceReference *sourceRef, OperandTypes &&...operands)                                 \
    {                                                                                                                  \
        std::initializer_list<MirOperand *> operandList = { std::forward<OperandTypes>(operands)... };                 \
        MirInstruction *instr = build(MirInstructionOpCode::NAME, sourceRef, operandList);                             \
                                                                                                                       \
        return instr;                                                                                                  \
    }                                                                                                                  \
    template <typename... OperandTypes> MirInstruction *NAME(OperandTypes &&...operands)                               \
    {                                                                                                                  \
        std::initializer_list<MirOperand *> operandList = { std::forward<OperandTypes>(operands)... };                 \
        MirInstruction *instr = build(MirInstructionOpCode::NAME, nullptr, operandList);                               \
                                                                                                                       \
        return instr;                                                                                                  \
    }

#include "Instruction/MirInstructionSetDefs.h"
#undef INSTRUCTION
    /**
     * Changes the insertion type of the current insertion point.
     */
    void changeInsertionType(InsertionType type);

    /**
     * Sets the insertion point for the builder.
     */
    void setInsertionPoint(MirInstructionInsertionPoint insertionPoint);

    /**
     * Sets the insertion point for the builder.
     */
    void setInsertionPoint(class MirBlock *block,
                           InsertionType type,
                           std::pmr::list<class MirInstruction *>::iterator it = {});

  private:
    /**
     * Creates an empty instruction container used the opcode and reference.
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode, SourceReference *ref);

    /**
     * Pushes the instruction into the proper place depending on the configuration of m_insertionPoint.
     */
    void finalizeInstruction(MirInstruction *instr, SourceReference *ref);

  private:
    class MirBuilderContext *m_ctx;
    MirInstructionInsertionPoint m_insertionPoint;
};

#endif // EZPACKER_MIR_INSTRUCTION_BUILDER_H
