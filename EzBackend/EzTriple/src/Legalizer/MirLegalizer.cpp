#include "Legalizer/MirLegalizer.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc), m_expandScalarAct(ctx, targetDesc), m_legalizeCallAct(ctx),
    m_legalizeRetAct(ctx), m_promoteScalarAct(ctx, targetDesc)
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

    if (opcode == MirInstructionOpCode::ALLOC)
    {
        // Allocs are used to create temporal space for objects / arrays. We don't need to legalise them.
        return MIRLEGALIZE_NO_ACTION;
    }
    else if (opcode == MirInstructionOpCode::CALL)
    {
        // A fully legalized call always looks like: [Token, Callee] (Exactly 2 operands).
        if (operands.size() != 2)
        {
            return &m_legalizeCallAct;
        }

        // Even with exactly 2 operands, it could still be an un-legalized call
        // with 1 parameter and no return value: [Callee, Arg0].
        // We verify that Operand 0 is a tracking token (Virtual/Physical register).
        if (operands[0]->isOfType<MirRegister>() &&
            operands[0]->get<MirRegister>()->getMirType()->getKind() == MirTypeKind::BindingToken)
        {
            return MIRLEGALIZE_NO_ACTION;
        }

        return &m_legalizeCallAct;
    }
    else if (opcode == MirInstructionOpCode::RET)
    {
        // A fully legalized RET instruction now holds exactly 1 tracking token: [retToken (type is BindingToken)]
        if (operands.size() == 1 && operands[0]->isOfType<MirRegister>() &&
            operands[0]->get<MirRegister>()->getMirType()->getKind() == MirTypeKind::BindingToken)
        {
            return MIRLEGALIZE_NO_ACTION;
        }

        // If it has operands that aren't a single token register (e.g., [Value]), legalize it.
        if (!operands.empty())
        {
            return &m_legalizeRetAct;
        }

        // Void return with 0 operands is inherently legal.
        return MIRLEGALIZE_NO_ACTION;
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
            return &m_expandScalarAct;
        }
    }

    return MIRLEGALIZE_NO_ACTION;
}

void MirLegalizer::addRule(LegalizeAction *action,
                           MirInstructionOpCode opcode,
                           std::vector<size_t> expectedOperandTypes)
{
    LegalizationRule rule{ .m_action = action, .m_expectedOperandTypes = std::move(expectedOperandTypes) };

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
        MirType *operandMirType = operands[i]->getMirType();
        size_t expectedType = rule.m_expectedOperandTypes[i];
        size_t operandType = operandMirType->getId();

        if (expectedType == MIRLEGALIZE_POINTER_TYPE)
        {
            if (operandMirType->getKind() != MirTypeKind::Pointer)
            {
                matched = false;
                break;
            }

            continue;
        }

        if (expectedType != MIRID_INVALID && expectedType != operandType)
        {
            matched = false;
            break;
        }
    }

    return matched;
}