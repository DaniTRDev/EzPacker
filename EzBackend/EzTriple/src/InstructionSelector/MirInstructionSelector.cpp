#include "InstructionSelector/MirInstructionSelector.h"

MirInstructionSelector::MirInstructionSelector(MirBuilderContext *ctx) :
    m_selectionRules(ctx->getGlobalAllocator()), m_alloc(ctx->getGlobalAllocator())
{
}

SelectionResult MirInstructionSelector::select(SelectionContext &ctx)
{
    MirInstruction *instr = *ctx.m_it;
    if (instr->isSelected())
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
            diag << "Executing selection rule" << instr->getSourceRef();
            diag.appendNote(std::format("Rule name: {}", rule.m_name).c_str(), nullptr);
            diag.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                            instr->getSourceRef());
            diag.flush();

            for (auto &action : rule.m_actions)
            {
                if (action(ctx) != SelectionResult::Selected)
                {
                    return SelectionResult::SelectionError;
                }
            }

            return SelectionResult::Selected;
        }
    }
    return SelectionResult::NoRule;
}

void MirInstructionSelector::addRule(MirInstructionOpCode opcode, InstructionSelectionRule rule)
{
    m_selectionRules[opcode].push_back(rule);
}

std::pmr::memory_resource *MirInstructionSelector::getAlloc() const { return m_alloc; }
