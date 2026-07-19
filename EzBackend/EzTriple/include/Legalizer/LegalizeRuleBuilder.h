#ifndef EZPACKER_LEGALIZERULEBUILDER_H
#define EZPACKER_LEGALIZERULEBUILDER_H

#include "EzTripleCommon.h"
#include "MirLegalizer.h"

class LegalizeRuleBuilder
{
  public:
    /**
     * Creates the builder attached to a legalizer that will receive the legalization rules created.
     * @param ctx
     * @param legaliszer
     */
    LegalizeRuleBuilder(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * Sets m_target and clears m_rules to start appending to a new rule.
     */
    LegalizeRuleBuilder &begin(MirInstructionOpCode target);

    /**
     * Builds a new rule with a custom predicate an action.
     */
    LegalizeRuleBuilder &custom(LegalizeRulePredicate pred, LegalizeAction *act);

    /**
     * Builds a new rule with the given predicate, if the predicate returns true in the target, an ExpandScalarAction
     * will be applied.
     */
    LegalizeRuleBuilder &expandIf(LegalizeRulePredicate pred);

    /**
     * Builds a new rule which will return true if the operand at given index of the instruction is of the given type.
     * If it is true, an ExpandScalarAction will be applied.
     */
    LegalizeRuleBuilder &expandType(size_t argIdx, MirType *type);

    /**
     * Builds a new rule with a predicate that will return true if the target instruction's destination operand(s) is
     * one of the given type list.
     *
     * Destination is retrieved through metadata (operands with Write flag set).
     */
    LegalizeRuleBuilder &legalForDest(std::vector<MirType *> types);

    /**
     * Builds a new rule with a predicate that will return true if the target instruction's source operand(s) is
     * one of the given type list.
     *
     * Destination is retrieved through metadata (operands with Read flag set and ReadWrite NOT SET).
     */
    LegalizeRuleBuilder &legalForSrc(std::vector<MirType *> types);

    /**
     * Same as legal for Scr / Dest but this will ONLY check the first operand because it assumes the instructions
     * operand types have the same type. To know which instructions can be used here, check in the instruction set for
     * those with "SameSize" constraint.
     *
     */
    LegalizeRuleBuilder &legalFor(std::vector<MirType *> types);

    /**
     * Builds a new rule with the given predicate.
     */
    LegalizeRuleBuilder &legalIf(LegalizeRulePredicate pred);

    /**
     * Builds a new rule that will return true if the arg at given index's size is less than the bit-width of the given
     * type. This will execute a PromoteScalarAction.
     */
    LegalizeRuleBuilder &minSize(size_t argIndex, MirType *type);

    /**
     * Dumps the rule into the given MirLegalizer by invoking its addRule method. A new rule will be added that will
     * return ILEGAL on any case to prevent undefined behaviour.
     */
    void dump();

  private:
    MirBuilderContext *m_ctx;
    MirLegalizer *m_legalizer;
    MirInstructionOpCode m_target;
    std::pmr::vector<LegalizeRule> m_rules;
};

#endif // EZPACKER_LEGALIZERULEBUILDER_H
