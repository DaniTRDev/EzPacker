#ifndef EZPACKER_MIRLEGALIZER_H
#define EZPACKER_MIRLEGALIZER_H

#include "EzTripleCommon.h"
#include "LegalizeAction.h"
#include "DefaultLegalizerActions/ExpandScalarAction.h"
#include "DefaultLegalizerActions/LegalizeCallAction.h"
#include "DefaultLegalizerActions/LegalizeReturnAction.h"
#include "DefaultLegalizerActions/PromoteScalarAction.h"

/**
 * This struct contains what's going to be checked to apply a legalization rule or not. This might be expanded in a
 * future to take in account more things.
 */
struct LegalizeRuleOperand
{
    MirBuilderContext *m_ctx{ nullptr };
    MirInstruction *m_instr{ nullptr };
};

using LegalizeRulePredicate = std::function<bool(const LegalizeRuleOperand &op)>;
inline LegalizeAction *LegalAction = reinterpret_cast<LegalizeAction *>(0);
inline LegalizeAction *IlegalAction = reinterpret_cast<LegalizeAction *>(-1);

struct LegalizeRule
{
    LegalizeAction *m_act; // Executed if m_predicate returns true.
    LegalizeRulePredicate m_predicate;
};

class MirLegalizer
{
  public:
    /**
     * Creates the legalizer with the given builder ctx and target descriptor attached.
     * @param diagnosticCollector
     * @param targetDesc
     */
    MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc);

    /**
     * Returns a pointer to the expand scalar action.
     */
    ExpandScalarAction *getExpandScalarAction();

    /**
     * Gets the specific action for a specific instruction.
     * Returns:
     *  - LegalAction if the legalizer mark this instruction as LEGAL.
     *  - IlegalAction if the legalizer doesn't know what to do with this instruction.
     *  - Another action if the legalizer knows how to transform this illegal instruction into a legal one.
     */
    LegalizeAction *getAction(MirInstruction *instr);

    /**
     * Returns the action used to legalize calls.
     */
    LegalizeCallAction *getCallAct();

    /**
     * Returns the action used to legalize returns.
     */
    LegalizeReturnAction *getReturnAct();

    /**
     * Returns a pointer to the promote scalar action.
     */
    PromoteScalarAction *getPromoteScalarAction();

    /**
     * Sets the rules of an opcode. This will OVERWRITE any other set of rules.
     */
    void addRule(MirInstructionOpCode opcode, std::pmr::vector<LegalizeRule> rules);

  private:
    // Define the default actions linked to the target and context.
    ExpandScalarAction m_expandScalarAct;
    LegalizeCallAction m_legalizeCallAct;
    LegalizeReturnAction m_legalizeRetAct;
    PromoteScalarAction m_promoteScalarAct;

  private:
    MirBuilderContext *m_ctx;
    TargetDesc *m_targetDesc;

    // Make searches faster by sorting the rules based on the opcode.
    std::map<MirInstructionOpCode, std::pmr::vector<LegalizeRule>> m_rules;
};

#endif // EZPACKER_MIRLEGALIZER_H
