#ifndef EZPACKER_MIRINSTRUCTIONBUILDER_H
#define EZPACKER_MIRINSTRUCTIONBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Printer/MirPrinter.h"

enum class InsertionType : uint8_t
{
    InsertAfter, // After a point.
    InsertBefore // Before a point.
};

/**
 * Structure that contains information about where the instruction produced by a builder should be stored.
 */
struct MirInstructionInsertionPoint
{
    InsertionType m_type;
    MirBlock *m_block;
    std::pmr::list<MirInstruction *>::iterator m_iterator{};
};

class MirInstructionBuilder : public MirBuilder<MirInstruction>
{
  public:
    /**
     * Creates the builder with the given ctx, insertion point and opcode.
     * @param ctx
     * @param insertionPoint
     */
    MirInstructionBuilder(MirBuilderContext *ctx, MirInstructionInsertionPoint insertionPoint);

    /**
     * Creates an instruction builder linked to a block at a specic position, the insertion order can be also set.
     * @param ctx
     * @param block
     * @param type
     * @param it
     */
    MirInstructionBuilder(MirBuilderContext *ctx,
                          MirBlock *block,
                          InsertionType type,
                          std::pmr::list<MirInstruction *>::iterator it);

    /**
     * Builds an instruction with the given opcode and inserts it with the insert point information.
     * @param opcode
     * @param ref
     * @param operands
     * @return
     */
    MirInstruction *
    build(MirInstructionOpCode opcode, SourceReference *ref, const std::initializer_list<MirOperand *> &operands = {});

    /**
     * Builds an instruction with the given opcode, operands and inserts it with the insert point information.
     * @param opcode
     * @param ref
     * @param operands
     * @return
     */
    MirInstruction *
    build(MirInstructionOpCode opcode, SourceReference *ref, const std::vector<MirOperand *> &operands = {});

    /**
     * Builds an instruction with the given opcode, operands and inserts it with the insert point information.
     * @param opcode
     * @param ref
     * @param operands
     * @return
     */
    MirInstruction *
    build(MirInstructionOpCode opcode, SourceReference *ref, const std::pmr::vector<MirOperand *> &operands);

    /**
     * Overload of the '<<' operator that allows pushing operands easily.
     * @param operand
     * @return
     */
    MirInstructionBuilder &operator<<(MirOperand *operand);

// Define the macro to generate a method for each instruction. This one makes possible attaching a source ref.
#define INSTRUCTION(NAME, tier, category, ops, flags)                                                                  \
    template <typename... OperandTypes> MirInstruction *NAME(SourceReference *sourceRef, OperandTypes &&...operands)   \
    {                                                                                                                  \
        std::initializer_list<MirOperand *> operandList = { std::forward<OperandTypes>(operands)... };                 \
        MirInstruction *instr = build(MirInstructionOpCode::NAME, sourceRef, operandList);                             \
                                                                                                                       \
        return instr;                                                                                                  \
    } // Include the file again to expand the macros

#include "Instruction/MirInstructionSet.h"
#undef INSTRUCTION

#define INSTRUCTION(NAME, tier, category, ops, flags)                                                                  \
    template <typename... OperandTypes> MirInstruction *NAME(OperandTypes &&...operands)                               \
    {                                                                                                                  \
        std::initializer_list<MirOperand *> operandList = { std::forward<OperandTypes>(operands)... };                 \
        MirInstruction *instr = build(MirInstructionOpCode::NAME, nullptr, operandList);                               \
                                                                                                                       \
        return instr;                                                                                                  \
    } // Include the file again to expand the macros

#include "Instruction/MirInstructionSet.h"
#undef INSTRUCTION

    /**
     * Changes the insertion type of the current insertion point.
     * @param type
     */
    void changeInsertionType(InsertionType type);

    /**
     * Sets the insertion point for the builder.
     * @param insertionPoint
     */
    void setInsertionPoint(MirInstructionInsertionPoint insertionPoint);

    /**
     * Sets the insertion point for the builder.
     * @param block
     * @param type
     * @param it
     */
    void setInsertionPoint(MirBlock *block, InsertionType type, std::pmr::list<MirInstruction *>::iterator it);

  private:
    MirBuilderContext *m_ctx;
    MirInstructionInsertionPoint m_insertionPoint;
};

#endif // EZPACKER_MIRINSTRUCTIONBUILDER_H
