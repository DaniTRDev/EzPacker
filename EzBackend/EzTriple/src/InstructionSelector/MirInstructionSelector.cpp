#include "InstructionSelector/MirInstructionSelector.h"

MirInstructionSelector::MirInstructionSelector(MirBuilderContext *ctx) : m_selectionRules(ctx->getGlobalAllocator()) {}

SelectionResult MirInstructionSelector::select(SelectionContext &ctx)
{
    MirInstruction *instr = *ctx.m_it;
    if (instr->getTargetId() != MIRID_INVALID)
    {
        return SelectionResult::AlreadySelected;
    }

    const auto &ruleIt = m_selectionRules.find(instr->getOpCode());
    if (ruleIt == m_selectionRules.end())
        return SelectionResult::NoRule;

    for (auto &rule : ruleIt->second)
    {
        if (rule.m_pred(ctx))
        {
            auto diag = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirInstructionSelector");
            diag << "Executing selection action" << instr->getSourceRef();
            diag.appendNote(std::format("Action name: {}", rule.m_name).c_str(), nullptr);
            diag.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                            instr->getSourceRef());
            diag.flush();
            return rule.m_act(ctx);
        }
    }
    return SelectionResult::NoRule;
}

void MirInstructionSelector::addRule(MirInstructionOpCode opcode, InstructionSelectionRule rule)
{
    m_selectionRules[opcode].push_back(rule);
}
