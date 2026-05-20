#ifndef EZPACKER_INSTRUCTIONSELECTIONTABLE_H
#define EZPACKER_INSTRUCTIONSELECTIONTABLE_H

#include "EzTargetCommon.h"

/**
 * Represents a single pattern matching rule for Instruction Selection.
 */
struct SelectionRule
{
    // The hardware ID to assign if this rule matches
    MirTargetInstructionId m_hardwareId{ TARGET_INSTR_SELECT_NONE };

    // E.g., { ExpectedOperandType::Register, ExpectedOperandType::Integer }
    std::vector<ExpectedOperandType> m_operandKinds;

    // E.g., { 1, 1 } for {i8, i8}. Sizes are in bytes.
    std::vector<size_t> m_operandSizes;

    /**
     * Checks if a given instruction matches this rule's constraints.
     */
    bool matches(class MirInstruction *instr) const;
};

class InstructionSelectionTable
{
  public:
    /**
     * Registers a new selection rule for a specific generic MIR opcode.
     */
    void addRule(MirInstructionOpCode opcode, const SelectionRule &rule);

    /**
     * Iterates through the registered rules for the instruction's opcode.
     * Returns the matched hardware ID, or TARGET_INST_NONE if no rule matched.
     */
    MirTargetInstructionId select(class MirInstruction *instr) const;

  private:
    std::unordered_map<MirInstructionOpCode, std::vector<SelectionRule>> m_rules;
};

#endif // EZPACKER_INSTRUCTIONSELECTIONTABLE_H
