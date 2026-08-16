#ifndef EZPACKER_MIRLEGALIZER_H
#define EZPACKER_MIRLEGALIZER_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"

/**
 * This struct contains what's going to be checked to apply a legalization rule or not as well as context-dependant
 * assets that need to be passed down different actions.
 */
struct LegalizeCtx
{
    MirBuilderContext *m_ctx{ nullptr };
    TargetDesc *m_targetDesc{ nullptr };
    std::pmr::list<MirInstruction *> *m_instrList{ nullptr };
    std::pmr::list<MirInstruction *>::iterator m_it;
    std::pmr::map<size_t, MirRegister *> m_promotionMap;                        // Map used to store promoted registers.
    std::pmr::map<size_t, std::pair<MirRegister *, MirRegister *>> m_expandMap; // orig, <low, high>
};

enum class LegalizationResult : uint8_t
{
    AlreadyLegal,     // The instruction was already legal.
    NoRule,           // The legalizer does not know how to legalize this instruction.
    Legalized,        // The instruction was legalized successfully.
    LegalizationError // There was an error during legalization in a particular action.
};

using LegalizeRulePredicate = std::function<bool(const LegalizeCtx &ctx)>;
using LegalizeRuleAction = std::function<LegalizationResult(LegalizeCtx &ctx)>;

struct LegalizeRule
{
    const char *m_name;       // Debugging purposes.
    LegalizeRuleAction m_act; // Executed if m_predicate returns true.
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
    MirLegalizer(MirBuilderContext *ctx);

    /**
     * Tries to legalize the instruction at ctx->instr. It will try one by one each rule and the first predicate that
     * evaluates to true is the one whose action will be executed.
     * @return
     */
    LegalizationResult legalize(LegalizeCtx &ctx);

    /**
     * Appends a rule for an opcode.
     * If priority is set to true, it will be appended in the FRONT.
     * If priority is set to false, it will be appended in the BACK.
     */
    void addRule(bool priority, MirInstructionOpCode opcode, const LegalizeRule &rule);

  private:
    MirBuilderContext *m_ctx;

    // Make searches faster by sorting the rules based on the opcode.
    std::pmr::map<MirInstructionOpCode, std::pmr::vector<LegalizeRule>> m_rules;
};

#endif // EZPACKER_MIRLEGALIZER_H
