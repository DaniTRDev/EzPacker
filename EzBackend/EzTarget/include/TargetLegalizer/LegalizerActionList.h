#ifndef EZPACKER_LEGALIZERACTIONLIST_H
#define EZPACKER_LEGALIZERACTIONLIST_H

#include "EzTargetCommon.h"

/**
 * Imagine this example from a RiscV32 architecture: RiscV32Legalizer() ->
 * All standard 32-bit math and bitwise ops are legal
 * setActionForClass(OpClass::Arithmetic, MirType::i32, LegalizeAction::Legal);
 * setActionForClass(OpClass::Bitwise,    MirType::i32, LegalizeAction::Legal);
 *
 * // All 8/16-bit math gets promoted to 32bit
 * setActionForClass(OpClass::Arithmetic, MirType::i8,  LegalizeAction::Promote);
 * setActionForClass(OpClass::Arithmetic, MirType::i16, LegalizeAction::Promote);
 *
 * // Override specific edge cases (Overrides the class rule)
 * setAction(MirOpcode::DIV, MirType::i32, LegalizeAction::Custom);
 */

/**
 * @brief The type of legalization action to perform for an illegal instruction or operand.
 */
enum TargetLegalizerActionType : uint8_t
{
    Action_None = (1 << 0), // The instruction is legal and does not require legalization.

    // Type
    Action_PromoteOperand = (1 << 1), // Promote the operand to a larger type (e.g., i32 to i64).
    Action_ExpandOperand = (1 << 2),  // Expand the operand into multiple smaller types (e.g., i64 to two i32).
    Action_TypeCustom = (1 << 3),     // Custom type legalization logic defined by the target.
};

class LegalizerActionList
{
  public:
    /**
     * Gets a set of legalization actions for a specific instruction and operand type.
     * @param opcode
     * @param typeId
     * @return
     */
    uint8_t getOperandAction(MirInstructionOpCode opcode, MirId typeId) const;

    /**
     * Sets the action for when a specific instruction is found with a specific destination operand type.
     * @param opcode
     * @param typeId
     * @param actionType
     */
    void setOperandAction(MirInstructionOpCode opcode, MirId typeId, uint8_t actionType);

    /**
     * Sets the action for when any instruction of a specific category is found with a specific operand type. This will
     * apply to all instructions of the category.
     * @param instrCategory
     * @param typeId
     * @param actionType
     */
    void setOperandActionForClass(MirInstructionCategory instrCategory, MirId typeId, uint8_t actionType);

  private:
    /*
     * Maps an opcode to a specific legalization action for a type. This allow defining a blind legalization action for
     * an entire class of instructions (e.g., all arithmetic instructions with i8 operands should be promoted to i32).
     *
     * For ex:
     * setOperandAction(ADD, i8, Action_PromoteOperand) -> Whenever an ADD instruction is found with an i8 operand, the
     * operand will be promoted.
     */
    std::array<std::map<size_t, uint8_t>, static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT)> m_operandActions;
};

#endif // EZPACKER_LEGALIZERACTIONLIST_H
