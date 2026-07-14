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

    if (opcode == MirInstructionOpCode::CALL)
    {
        // Case 1: [Callee] (Void function call -> size == 1)
        // Case 2: [Token, Callee] (Function returning a value -> size == 2)
        // Anything larger than 2 operands means arguments or destination registers haven't been popped out yet.
        if (operands.size() > 2)
        {
            return &m_legalizeCallAct;
        }

        // [DestReg, Callee] (un-legalized, 0-argument call with return)
        // Or it could be [TokenReg, Callee] (fully legalized call with return value).
        if (operands.size() == 2)
        {
            // If the first operand is a Register, it's the returnt token, meaning it has already been legalized.
            if (operands[0]->isOfType<MirRegister>())
            {
                return MIRLEGALIZE_NO_ACTION;
            }

            // If operands[0] is NOT a destination register (e.g., it's a direct reference/symbol),
            // then a size of 2 means [Callee, Arg0], which definitely needs legalization.
            return &m_legalizeCallAct;
        }

        // If size is exactly 1 ([Callee]), it's a fully legalized void call.
        return MIRLEGALIZE_NO_ACTION;
    }
    else if (opcode == MirInstructionOpCode::RET)
    {
        if (!operands.empty())
        {
            // Only return the action for RET that don't have been legalized yet.
            return &m_legalizeRetAct;
        }
        else
        {
            // This call is already legal.
            return MIRLEGALIZE_NO_ACTION;
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
