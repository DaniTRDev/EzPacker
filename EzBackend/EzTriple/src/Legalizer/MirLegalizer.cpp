#include "Legalizer/MirLegalizer.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc), m_expandScalarAct(ctx, targetDesc), m_legalizeCallAct(ctx),
    m_legalizeRetAct(ctx), m_promoteScalarAct(ctx, targetDesc)
{
}

ExpandScalarAction *MirLegalizer::getExpandScalarAction() { return &m_expandScalarAct; }

LegalizeAction *MirLegalizer::getAction(MirInstruction *instr)
{
    // Look up the registered rule sequence for this specific opcode
    auto it = m_rules.find(instr->getOpCode());
    if (it == m_rules.end())
    {
        // No explicit rules registered; fall back to the default error/unsupported handler
        return IlegalAction;
    }

    LegalizeRuleOperand op{ .m_ctx = m_ctx, .m_instr = instr };

    // Evaluate rules in insertion order (First-Match Wins Strategy)
    for (const auto &rule : it->second)
    {
        if (rule.m_predicate(op))
        {
            return rule.m_act;
        }
    }

    // Revert to fallback if none of the rule filters matched the instruction layout context
    return IlegalAction;
}

LegalizeCallAction *MirLegalizer::getCallAct() { return &m_legalizeCallAct; }

LegalizeReturnAction *MirLegalizer::getReturnAct() { return &m_legalizeRetAct; }

PromoteScalarAction *MirLegalizer::getPromoteScalarAction() { return &m_promoteScalarAct; }

void MirLegalizer::addRule(MirInstructionOpCode opcode, std::pmr::vector<LegalizeRule> rules)
{
    m_rules[opcode] = std::move(rules);
}