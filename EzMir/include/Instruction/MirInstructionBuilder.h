#ifndef EZPACKER_MIR_INSTRUCTION_BUILDER_H
#define EZPACKER_MIR_INSTRUCTION_BUILDER_H

#include "EzMirCommon.h"
#include "MirInstructionSet.h"
#include "Builder/MirBuilder.h"
#include "HelperClasses/IntrusiveLinkedList.h"

/**
 * Mode specifying how newly constructed instructions are inserted into a basic block.
 */
enum class InsertionType : uint8_t
{
    InsertAfter,  // Insert after the cursor iterator position
    InsertBefore, // Insert before the cursor iterator position
    Append        // Append to the end of the block's instruction list
};

/**
 * State describing the insertion cursor within a basic block.
 */
struct MirInstructionInsertionPoint
{
    /**
     * Placement mode (Append, InsertBefore, InsertAfter).
     */
    InsertionType m_type;

    /**
     * Target basic block receiving constructed instructions.
     */
    class MirBlock *m_block;

    /**
     * List iterator cursor position for InsertBefore / InsertAfter modes.
     */
    IntrusiveLinkedList<class MirInstruction>::iterator m_iterator{};
};

/**
 * High-level and target instruction builder.
 * Constructs MirInstruction objects in the context memory arena, attaches operands,
 * validates constraints, and splices instructions into basic blocks at configured insertion points.
 */
class MirInstructionBuilder : public MirBuilder<class MirInstruction>
{
  public:
    /**
     * Constructs an instruction builder with an explicit insertion point cursor.
     */
    MirInstructionBuilder(class MirBuilderContext *ctx, MirInstructionInsertionPoint insertionPoint);

    /**
     * Constructs an instruction builder positioned at the specified block, insertion mode, and iterator.
     */
    MirInstructionBuilder(class MirBuilderContext *ctx,
                          class MirBlock *block,
                          InsertionType type,
                          IntrusiveLinkedList<MirInstruction>::iterator it = {});

    /**
     * Builds and inserts an instruction with opcode, source ref, and initializer list of operand pointers.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::initializer_list<class MirOperand *> &operands = {});

    /**
     * Builds and inserts an instruction with opcode, source ref, and std::vector of operand pointers.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::vector<class MirOperand *> &operands = {});

    /**
     * Builds and inserts an instruction with opcode, source ref, and polymorphic vector of operand pointers.
     */
    MirInstruction *build(MirInstructionOpCode opcode,
                          class SourceReference *ref,
                          const std::pmr::vector<class MirOperand *> &operands);

    /**
     * Builds and inserts a target-specific machine instruction (TARGET_INST) bound to a MirTargetInstructionDesc.
     */
    MirInstruction *buildTarget(class MirTargetInstructionDesc *targetDesc,
                                class SourceReference *srcRef,
                                std::initializer_list<class MirOperand *> operands);

    /**
     * Appends an operand to the instruction currently being constructed.
     */
    MirInstructionBuilder &operator<<(class MirOperand *operand);

// Define the macro to generate a method for each instruction opcode (e.g. ADD, SUB, MOV, BR, CALL, RET).
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
     * Updates the insertion mode (Append, InsertBefore, InsertAfter) at the current cursor.
     */
    void changeInsertionType(InsertionType type);

    /**
     * Sets the complete insertion point cursor structure.
     */
    void setInsertionPoint(MirInstructionInsertionPoint insertionPoint);

    /**
     * Sets the target block, insertion mode, and iterator cursor position.
     */
    void setInsertionPoint(class MirBlock *block,
                           InsertionType type,
                           IntrusiveLinkedList<class MirInstruction>::iterator it = {});

  private:
    /**
     * Allocates an unlinked MirInstruction instance in the arena allocator.
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode, SourceReference *ref);

    /**
     * Logs diagnostic trace and splices the instruction into the basic block intrusive list.
     */
    void finalizeInstruction(MirInstruction *instr, SourceReference *ref);

  private:
    /**
     * Context providing memory resources and diagnostic logging.
     */
    class MirBuilderContext *m_ctx;

    /**
     * Current cursor position and insertion strategy.
     */
    MirInstructionInsertionPoint m_insertionPoint;
};

#endif // EZPACKER_MIR_INSTRUCTION_BUILDER_H
