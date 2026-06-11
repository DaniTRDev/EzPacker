#include "Legalizer/MirLegalizer.h"

MirLegalizer::MirLegalizer(DiagnosticCollector *diagnosticCollector) : m_diagnosticCollector(diagnosticCollector) {}

LegalizeActionResult MirLegalizer::executeAction(std::pmr::list<class MirInstruction *> &instrList,
                                                 std::pmr::list<class MirInstruction *>::iterator it,
                                                 LegalizeAction *action)
{
    MirInstruction *instr = *it;

    if (action == MIRLEGALIZE_NO_ACTION)
    {
        auto log = m_diagnosticCollector->builder(Diag_Trace, "MirLegalizer");
        log << "LEGAL";
        log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(), instr->getSourceRef());

        return { .m_executed = true, .m_succeeded = true, .m_mirChanged = false };
    }
    else if (!action)
    {
        auto log = m_diagnosticCollector->builder(Diag_Error, "MirLegalizer");
        log << "Invalid action given for the instruction";
        log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(), instr->getSourceRef());

        return { .m_executed = false, .m_succeeded = false, .m_mirChanged = false };
    }

    return action->run(instrList, it);
}

LegalizeAction *MirLegalizer::getAction(MirInstructionOpCode opcode, const std::pmr::vector<MirOperand *> &operands)
{
    auto it = m_rules.find(opcode);
    if (it == m_rules.end())
        return nullptr;

    for (const auto &rule : it->second)
    {
        if (rule.m_expectedOperandTypes.size() != operands.size())
        {
            continue;
        }

        LegalizeAction *action = rule.m_action;
        for (size_t i = 0; i < operands.size(); i++)
        {
            size_t expectedType = rule.m_expectedOperandTypes[i];
            size_t operandType = operands[i]->getMirType()->getId();

            if (expectedType != operandType)
            {
                action = nullptr;
                break;
            }
        }

        if (action != nullptr)
            return action;
    }

    return nullptr;
}

void MirLegalizer::addRule(LegalizeAction *action,
                           MirInstructionOpCode opcode,
                           std::vector<size_t> expectedOperandTypes)
{
    LegalizationRule rule{ .m_action = action,
                           .m_opcode = opcode,
                           .m_expectedOperandTypes = std::move(expectedOperandTypes) };

    auto it = m_rules.find(opcode);
    if (it != m_rules.end())
    {
        it->second.push_back(std::move(rule));
    }
    else
    {
        m_rules[opcode] = { rule };
    }
}

void MirLegalizer::addRuleForCategory(LegalizeAction *action,
                                      MirInstructionCategory category,
                                      std::vector<size_t> expectedOperandTypes)
{
    for (auto &meta : g_MirInstructionSet)
    {
        if (meta.m_category == category)
            addRule(action, meta.m_opcode, std::move(expectedOperandTypes));
    }
}
