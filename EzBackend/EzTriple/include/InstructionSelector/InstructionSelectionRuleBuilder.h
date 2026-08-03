#ifndef EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H
#define EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H

#include "EzTripleCommon.h"
#include "MirInstructionSelector.h"

class InstructionSelectionRuleBuilder
{
  public:
    /**
     * Creates the rule builder linked to the given selector.
     * @param selector
     */
    InstructionSelectionRuleBuilder(MirInstructionSelector *selector);
    
    /**
     * Sets the action for the current target instruction rule. WILL OVERWRITE THE CURRENT ACTION.
     * @param act
     * @return
     */
    InstructionSelectionRuleBuilder &act(const InstructionSelAction &act);
    
    /**
     * Begins a new rule with the given name and targeting the given opcode.
     * @param name
     * @param opcode
     * @return
     */
    InstructionSelectionRuleBuilder &begin(const char *name, MirInstructionOpCode opcode);

    /**
     * Sets the predicate for the current target instruction rule. WILL OVERWRITE THE CURRENT PRED.
     * @param pred
     * @return
     */
    InstructionSelectionRuleBuilder &pred(const InstructionSelPred &pred);

    /**
     * Pushes the current rule to the selector. Will reset the target opcode to INVALID.
     */
    void dump();

  private:
    InstructionSelectionRule m_rule;
    MirInstructionOpCode m_targetOpCode;
    MirInstructionSelector *m_selector;
};

#endif // EZPACKER_INSTRUCTION_SELECTION_RULE_BUILDER_H