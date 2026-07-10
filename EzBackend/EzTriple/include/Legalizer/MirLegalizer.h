#ifndef EZPACKER_MIRLEGALIZER_H
#define EZPACKER_MIRLEGALIZER_H

#include "EzTripleCommon.h"
#include "LegalizeAction.h"
#include "DefaultLegalizerActions/PromoteScalarAction.h"
#include "DefaultLegalizerActions/ExpandScalarAction.h"

struct LegalizationRule
{
    LegalizeAction *m_action;
    MirInstructionOpCode m_opcode;
    std::vector<size_t> m_expectedOperandTypes; /**
                                                 * List of MirType id's that are expected for this instruction. Only
                                                 * the types specified here will be checked. If id == MIRID_INVALID,
                                                 * it will be skipped.
                                                 */
};

inline LegalizeAction *MIRLEGALIZE_NO_ACTION = reinterpret_cast<LegalizeAction *>(-1);

class MirLegalizer
{
  public:
    /**
     * Creates the legalizer with the given ctx and target descriptor attached.
     * @param diagnosticCollector
     * @param targetDesc
     */
    MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc);

    /**
     * Gets the specific action (set through addRule) for a specific combo of opcode + operands types. Returns
     * MIRLEGALIZE_NO_ACTION if the legalizer mark this combo as LEGAL.
     *
     * If there's no action set for this combo, a default action will try to be invoked:
     *  - Promotion
     *  - Expansion
     * @param opcode
     * @param operands
     * @return
     */
    LegalizeAction *getAction(MirInstructionOpCode opcode, const std::pmr::vector<MirOperand *> &operands);

    /**
     * Adds a rule that executes an action on a match.
     * @param action
     * @param opcode
     * @param expectedOperandTypes
     */
    void addRule(LegalizeAction *action, MirInstructionOpCode opcode, std::vector<size_t> expectedOperandTypes);

    /**
     * Adds a rule for EVERY INSTRUCTION inside the category.
     * @param action
     * @param category
     * @param expectedOperandTypes
     */
    void addRuleForCategory(LegalizeAction *action,
                            MirInstructionCategory category,
                            std::vector<size_t> expectedOperandTypes);

  private:
    /**
     * Returns true if the given opcode and set of operands matches the given rule's restrictions.
     * @param rule
     * @param operands
     * @return
     */
    bool matchOperands(const LegalizationRule &rule, const std::pmr::vector<MirOperand *> &operands);

  private:
    // Define the default actions linked to the target and context.
    ExpandScalarAction m_expandScalarAct;
    PromoteScalarAction m_promoteScalarAct;

  private:
    MirBuilderContext *m_ctx;
    TargetDesc *m_targetDesc;
    // Make searches faster by sorting the rules based on the opcode.
    std::map<MirInstructionOpCode, std::vector<LegalizationRule>> m_rules;
};

#endif // EZPACKER_MIRLEGALIZER_H
