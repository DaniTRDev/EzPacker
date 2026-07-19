#include "Legalizer/LegalizeRuleBuilder.h"

LegalizeRuleBuilder::LegalizeRuleBuilder(MirBuilderContext *ctx, MirLegalizer *legalizer) :
    m_ctx(ctx), m_legalizer(legalizer), m_rules(ctx->getGlobalAllocator())
{
}

LegalizeRuleBuilder &LegalizeRuleBuilder::begin(MirInstructionOpCode target)
{
    m_target = target;
    m_rules.clear();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::custom(LegalizeRulePredicate pred, LegalizeAction *act)
{
    m_rules.push_back(LegalizeRule{ .m_act = act, .m_predicate = pred });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::expandIf(LegalizeRulePredicate pred)
{
    m_rules.push_back(LegalizeRule{ .m_act = m_legalizer->getExpandScalarAction(), .m_predicate = std::move(pred) });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::expandType(size_t argIdx, MirType *type)
{
    m_rules.push_back(LegalizeRule{ .m_act = m_legalizer->getExpandScalarAction(),
                                    .m_predicate = [argIdx, type](const LegalizeRuleOperand &op) -> bool
                                    {
                                        const auto &instr = op.m_instr;
                                        const auto &operands = instr->getOperands();

                                        if (operands.size() <= argIdx)
                                            return false;

                                        const auto &operand = operands[argIdx];
                                        return operand->getMirType()->getId() == type->getId();
                                    } });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalForDest(std::vector<MirType *> types)
{
    m_rules.push_back(LegalizeRule{ .m_act = LegalAction,
                                    .m_predicate = [types](const LegalizeRuleOperand &op) -> bool
                                    {
                                        const auto &instr = op.m_instr;
                                        const auto &instrMetadata = instr->getMetadata();
                                        const auto &operands = instr->getOperands();

                                        bool checkedAny = false;
                                        for (size_t i = 0; i < operands.size(); i++)
                                        {
                                            const auto &constraint = instrMetadata.m_operandConstraints[i];
                                            const auto &operand = operands[i];

                                            if (constraint.flags & OperandFlag::Write)
                                            {
                                                checkedAny = true;
                                                bool match = false;
                                                for (auto legalType : types)
                                                {
                                                    if (operand->getMirType()->getId() == legalType->getId())
                                                    {
                                                        match = true;
                                                        break;
                                                    }
                                                }

                                                if (!match)
                                                {
                                                    return false; // Immediate rejection of invalid write type layout
                                                }
                                            }
                                        }

                                        // If we found zero destination writes to verify, return false so alternative
                                        // rules can evaluate this instruction. Otherwise, all checks passed.
                                        return checkedAny;
                                    } });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalForSrc(std::vector<MirType *> types)
{
    m_rules.push_back(LegalizeRule{
            .m_act = LegalAction,
            .m_predicate = [types](const LegalizeRuleOperand &op) -> bool
            {
                const auto &instr = op.m_instr;
                const auto &instrMetadata = instr->getMetadata();
                const auto &operands = instr->getOperands();

                bool checkedAny = false;
                for (size_t i = 0; i < operands.size(); i++)
                {
                    const auto &constraint = instrMetadata.m_operandConstraints[i];
                    const auto &operand = operands[i];

                    if ((constraint.flags & OperandFlag::Read) && !(constraint.flags & OperandFlag::ReadWrite))
                    {
                        checkedAny = true;
                        bool match = false;
                        for (auto &legalType : types)
                        {
                            if (operand->getMirType()->getId() == legalType->getId())
                            {
                                match = true;
                                break;
                            }
                        }

                        if (!match)
                        {
                            return false; // Immediate rejection of illegal source variable layout
                        }
                    }
                }

                // If no read-only source operands exist, pass evaluation back to alternative rules.
                return checkedAny;
            } });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalFor(std::vector<MirType *> types)
{
    m_rules.push_back(LegalizeRule{ .m_act = LegalAction,
                                    .m_predicate = [types](const LegalizeRuleOperand &op) -> bool
                                    {
                                        const auto &instr = op.m_instr;
                                        const auto &operands = instr->getOperands();

                                        if (operands.empty())
                                            return false;

                                        const auto &operand = operands[0];
                                        for (auto &legalType : types)
                                        {
                                            if (operand->getMirType()->getId() == legalType->getId())
                                                return true;
                                        }

                                        return false;
                                    } });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalIf(LegalizeRulePredicate pred)
{
    m_rules.push_back(LegalizeRule{ .m_act = LegalAction, .m_predicate = std::move(pred) });
    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::minSize(size_t argIndex, MirType *type)
{
    m_rules.push_back(LegalizeRule{ .m_act = m_legalizer->getPromoteScalarAction(),
                                    .m_predicate = [argIndex, type](const LegalizeRuleOperand &op) -> bool
                                    {
                                        const auto &instr = op.m_instr;
                                        const auto &operands = instr->getOperands();

                                        if (operands.size() <= argIndex)
                                            return false;

                                        const auto &operand = operands[argIndex];
                                        return operand->getMirType()->getTotalSizeInBits() < type->getTotalSizeInBits();
                                    } });
    return *this;
}

void LegalizeRuleBuilder::dump()
{
    m_rules.push_back(LegalizeRule{ .m_act = IlegalAction,
                                    .m_predicate = [](const LegalizeRuleOperand &op) -> bool { return true; } });

    m_legalizer->addRule(m_target, std::move(m_rules));
    m_target = MirInstructionOpCode::INVALID;
}