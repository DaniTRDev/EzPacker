#include "Legalizer/LegalizeRuleBuilder.h"

LegalizeRuleBuilder::LegalizeRuleBuilder(MirLegalizer *legalizer) : m_legalizer(legalizer) {}

LegalizeRuleBuilder &LegalizeRuleBuilder::begin(const char *name, MirInstructionOpCode target, bool priority)
{
    m_priority = priority;
    m_name = name;
    m_target = target;

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::custom(const LegalizeRulePredicate &pred, const LegalizeRuleAction &act)
{
    m_rule = LegalizeRule{ .m_name = m_name, .m_act = act, .m_predicate = pred };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::expandIf(const LegalizeRulePredicate &pred)
{
    m_rule = LegalizeRule{ .m_name = m_name, .m_act = LegalizeActions::ExpandScalar, .m_predicate = std::move(pred) };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::expandType(size_t argIdx, MirType *type)
{
    m_rule = LegalizeRule{ .m_name = m_name,
                           .m_act = LegalizeActions::ExpandScalar,
                           .m_predicate = [argIdx, type](const LegalizeCtx &ctx) -> bool
                           {
                               const auto &instr = *ctx.m_it;
                               const auto &operands = instr->getOperands();

                               if (operands.size() <= argIdx)
                                   return false;

                               const auto &operand = operands[argIdx];
                               return operand->getMirType()->getId() == type->getId();
                           } };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalForDest(std::vector<MirType *> types)
{
    m_rule = LegalizeRule{ .m_name = m_name,
                           .m_act = LegalizeActions::Legal,
                           .m_predicate = [types](const LegalizeCtx &ctx) -> bool
                           {
                               const auto &instr = *ctx.m_it;
                               const auto &instrMetadata = instr->getMetadata();
                               const auto &operands = instr->getOperands();

                               bool checkedAny = false;
                               for (size_t i = 0; i < operands.size(); i++)
                               {
                                   const auto &constraint = instrMetadata.m_operandFlags[i];
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
                           } };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalForSrc(std::vector<MirType *> types)
{
    m_rule = LegalizeRule{ .m_name = m_name,
                           .m_act = LegalizeActions::Legal,
                           .m_predicate = [types](const LegalizeCtx &ctx) -> bool
                           {
                               const auto &instr = *ctx.m_it;
                               const auto &instrMetadata = instr->getMetadata();
                               const auto &operands = instr->getOperands();

                               bool checkedAny = false;
                               for (size_t i = 0; i < operands.size(); i++)
                               {
                                   const auto &constraint = instrMetadata.m_operandFlags[i];
                                   const auto &operand = operands[i];

                                   if ((constraint.flags & OperandFlag::Read) &&
                                       !(constraint.flags & OperandFlag::ReadWrite))
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
                           } };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalFor(std::vector<MirType *> types)
{
    m_rule = LegalizeRule{ .m_name = m_name,
                           .m_act = LegalizeActions::Legal,
                           .m_predicate = [types](const LegalizeCtx &ctx) -> bool
                           {
                               const auto &instr = *ctx.m_it;
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
                           } };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::legalIf(const LegalizeRulePredicate &pred)
{
    m_rule = LegalizeRule{ .m_name = m_name, .m_act = LegalizeActions::Legal, .m_predicate = std::move(pred) };
    dump();

    return *this;
}

LegalizeRuleBuilder &LegalizeRuleBuilder::minSize(size_t argIndex, MirType *type)
{
    m_rule = LegalizeRule{ .m_name = m_name,
                           .m_act = LegalizeActions::PromoteScalar,
                           .m_predicate = [argIndex, type](const LegalizeCtx &ctx) -> bool
                           {
                               const auto &instr = *ctx.m_it;
                               const auto &operands = instr->getOperands();

                               if (operands.size() <= argIndex)
                                   return false;

                               const auto &operand = operands[argIndex];
                               return operand->getMirType()->getTotalSizeInBits() < type->getTotalSizeInBits();
                           } };
    dump();

    return *this;
}

void LegalizeRuleBuilder::dump() { m_legalizer->addRule(m_priority, m_target, m_rule); }