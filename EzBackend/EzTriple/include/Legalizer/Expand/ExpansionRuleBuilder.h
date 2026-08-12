#ifndef EZPACKER_EXPANSIONRULEBUILDER_H
#define EZPACKER_EXPANSIONRULEBUILDER_H

#include "EzTripleCommon.h"
#include "MirExpansionRuleRegistry.h"

class ExpansionRuleBuilder
{
  public:
    /**
     * Creates a new rule builder linked to the given registry. The new rule's target is the
     * given opcode.
     */
    ExpansionRuleBuilder(MirExpansionRuleRegistry *registry, MirInstructionOpCode target);

    /**
     * If the rule was not dumped, it will be dumped into the linked registry.
     */
    ~ExpansionRuleBuilder();

    /**
     * Emits an instruction with the given operands.
     */
    ExpansionRuleBuilder &emit(MirInstructionOpCode opcode, std::vector<ExpansionOperand> operands);

    /**
     * Emits a CALL instruction with a runtime symbol operand.
     */
    ExpansionRuleBuilder &emit(const std::string_view &rtSymbol, std::vector<ExpansionOperand> operands);

    /**
     * Adds a predicate to the current rule. This will make the rule ONLY execute if the predicate returned true.
     */
    ExpansionRuleBuilder &pred(ExpansionPredicate pred);

    /**
     * Dumps the current rule into the registry.
     */
    void dump();

  private:
    ExpansionRule m_rule;
    MirExpansionRuleRegistry *m_registry;
    MirInstructionOpCode m_target;
};

#endif // EZPACKER_EXPANSIONRULEBUILDER_H