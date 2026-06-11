#ifndef EZPACKER_MIRLEGALIZER_H
#define EZPACKER_MIRLEGALIZER_H

#include "EzTripleCommon.h"
#include "LegalizeAction.h"

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
     * Creates the legalizer with the given diagnostics collector attached.
     * @param diagnosticCollector
     */
    MirLegalizer(DiagnosticCollector *diagnosticCollector);

    /**
     * Executes an action in an instruction and returns the result. This method SUPPOSES that getAction was called
     * before and returned the 'action' pointer correctly.
     * @param instrList
     * @param it
     * @param action
     * @return
     */
    LegalizeActionResult executeAction(std::pmr::list<class MirInstruction *> &instrList,
                                       std::pmr::list<class MirInstruction *>::iterator it,
                                       LegalizeAction *action);

    /**
     * Gets the action for a specific combo of opcode + operands types. Returns MIRLEGALIZE_NO_ACTION if the legalizer
     * mark this combo as LEGAL.
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
    DiagnosticCollector *m_diagnosticCollector;
    // Make searches faster by sorting the rules based on the opcode.
    std::map<MirInstructionOpCode, std::vector<LegalizationRule>> m_rules;
};

#endif // EZPACKER_MIRLEGALIZER_H
