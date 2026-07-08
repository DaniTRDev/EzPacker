#include "Legalizer/MirLegalizer.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc), m_promoteScalarAct(ctx, targetDesc)
{
}

LegalizeAction *MirLegalizer::getAction(MirInstructionOpCode opcode, const std::pmr::vector<MirOperand *> &operands)
{
    auto it = m_rules.find(opcode);
    if (it != m_rules.end())
    {

        for (const auto &rule : it->second)
        {
            if (matchOperands(rule, operands))
            {
                return rule.m_action;
            }
        }
    }

    for (MirOperand *op : operands)
    {
        MirType *type = op->getMirType();
        MirType *legalType = m_targetDesc->getNearestLegalType(type);

        if (!legalType)
        {
            auto log = m_ctx->getDiagCollector()->builder(Diag_Error, "MirLegalizer");
            log << "Legalizer doesn't know what rule to apply for operand";
            log.appendNote(MirPrinter::printToString(op).c_str(), op->getSourceRef());

            return nullptr;
        }

        if (legalType->getTotalSizeInBits() > type->getTotalSizeInBits())
        {
            // Promotion.
            return &m_promoteScalarAct;
        }
        else if (legalType->getTotalSizeInBits() < type->getTotalSizeInBits())
        {
            // Expansion.
            return nullptr; // TO BE DONE. TODO
        }
    }

    return MIRLEGALIZE_NO_ACTION;
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
            addRule(action, meta.m_opcode, expectedOperandTypes);
    }
}

bool MirLegalizer::matchOperands(const LegalizationRule &rule, const std::pmr::vector<MirOperand *> &operands)
{
    if (rule.m_expectedOperandTypes.size() != operands.size())
    {
        return false;
    }

    bool matched = true;
    for (size_t i = 0; i < operands.size(); i++)
    {
        size_t expectedType = rule.m_expectedOperandTypes[i];
        size_t operandType = operands[i]->getMirType()->getId();

        if (expectedType != MIRID_INVALID && expectedType != operandType)
        {
            matched = false;
            break;
        }
    }

    return matched;
}
