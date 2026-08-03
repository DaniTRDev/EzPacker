#include "Legalizer/MirLegalizer.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx) : m_ctx(ctx), m_rules(m_ctx->getGlobalAllocator()) {}

LegalizationResult MirLegalizer::legalize(LegalizeCtx &ctx)
{
    // Look up the registered rule sequence for this specific opcode
    MirInstruction *instr = *ctx.m_it;
    auto it = m_rules.find(instr->getOpCode());
    if (it == m_rules.end())
    {
        // No explicit rules registered; fall back to the default error/unsupported handler
        return LegalizationResult::NoRule;
    }

    // Evaluate rules in insertion order (First-Match Wins Strategy)
    for (const auto &rule : it->second)
    {
        if (rule.m_predicate(ctx))
        {
            auto diag = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizer");
            diag << "Executing legalization action";
            diag.appendNote(std::format("Action name: {}", rule.m_name).c_str(), nullptr);
            diag.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                            instr->getSourceRef());
            diag.flush();

            return rule.m_act(ctx);
        }
    }

    // Revert to fallback if none of the rule filters matched the instruction layout context
    return LegalizationResult::NoRule;
}

void MirLegalizer::addRule(bool priority, MirInstructionOpCode opcode, const LegalizeRule &rule)
{
    if (priority)
        m_rules[opcode].insert(m_rules[opcode].begin(), rule);
    else
        m_rules[opcode].push_back(rule);
}