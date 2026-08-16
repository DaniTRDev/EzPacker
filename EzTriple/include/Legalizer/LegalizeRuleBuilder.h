#ifndef EZPACKER_LEGALIZERULEBUILDER_H
#define EZPACKER_LEGALIZERULEBUILDER_H

#include "EzTripleCommon.h"
#include "Actions/ExpandScalarAction.h"
#include "Actions/LegalAction.h"
#include "Actions/LegalizeCallAction.h"
#include "Actions/LegalizeReturnAction.h"
#include "Actions/PromoteScalarAction.h"

class LegalizeRuleBuilder
{
  public:
    /**
     * Creates the builder attached to a legalizer that will receive the legalization rules created.
     * @param ctx
     * @param legalizer
     */
    LegalizeRuleBuilder(MirLegalizer *legalizer);

    /**
     * Sets m_target and m_name overriding its values. If priority is set to true, this rule will be inserted in the
     * FRONT, meaning it will be the first rule to be matched.
     */
    LegalizeRuleBuilder &begin(const char *name, MirInstructionOpCode target, bool priority = false);

    /**
     * Builds a new rule with a custom predicate an action. It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &custom(const LegalizeRulePredicate &pred, const LegalizeRuleAction &act);

    /**
     * Builds a new rule with the given predicate, if the predicate returns true in the target, an ExpandScalarAction
     * will be applied. It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &expandIf(const LegalizeRulePredicate &pred);

    /**
     * Builds a new rule which will return true if the operand at given index of the instruction is of the given type.
     * If it is true, an ExpandScalarAction will be applied. It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &expandType(size_t argIdx, MirType *type);

    /**
     * Builds a new rule with a predicate that will return true if the target instruction's destination operand(s) is
     * one of the given type list. It will also call dump to commit the rule to the legalizer.
     *
     * Destination is retrieved through metadata (operands with Write flag set).
     */
    LegalizeRuleBuilder &legalForDest(std::vector<MirType *> types);

    /**
     * Builds a new rule with a predicate that will return true if the target instruction's source operand(s) is
     * one of the given type list. It will also call dump to commit the rule to the legalizer.
     *
     * Destination is retrieved through metadata (operands with Read flag set and ReadWrite NOT SET).
     */
    LegalizeRuleBuilder &legalForSrc(std::vector<MirType *> types);

    /**
     * Same as legal for Scr / Dest but this will ONLY check the first operand because it assumes the instructions
     * operand types have the same type. To know which instructions can be used here, check in the instruction set for
     * those with "SameSize" constraint.
     *
     * It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &legalFor(std::vector<MirType *> types);

    /**
     * Builds a new rule with the given predicate. It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &legalIf(const LegalizeRulePredicate &pred);

    /**
     * Builds a new rule that will return true if the arg at given index's size is less than the bit-width of the given
     * type. This will execute a PromoteScalarAction.
     *
     * It will also call dump to commit the rule to the legalizer.
     */
    LegalizeRuleBuilder &minSize(size_t argIndex, MirType *type);

    /**
     * Dumps the rule into the given MirLegalizer by invoking its addRule method. If priority is set to true, the
     * rule will be appended in the FRONT, making it the first rule to be checked.
     */
    void dump();

  private:
    bool m_priority;
    const char *m_name;
    MirLegalizer *m_legalizer;
    MirInstructionOpCode m_target;
    LegalizeRule m_rule;
};

#endif // EZPACKER_LEGALIZERULEBUILDER_H
