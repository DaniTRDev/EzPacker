#include "InstructionSelector/InstructionSelectionRuleBuilder.h"

InstructionSelectionRuleBuilder::InstructionSelectionRuleBuilder(MirInstructionSelector *selector) :
    m_rule({}), m_targetOpCode(MirInstructionOpCode::INVALID), m_selector(selector)
{
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::act(const InstructionSelAction &act)
{
    m_rule.m_act = act;
    return *this;
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::begin(const char *name, MirInstructionOpCode opcode)
{
    m_targetOpCode = opcode;
    m_rule = InstructionSelectionRule{ .m_name = name, .m_act = {}, .m_pred = {} };
    return *this;
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::pred(const InstructionSelPred &pred)
{
    m_rule.m_pred = pred;
    return *this;
}

void InstructionSelectionRuleBuilder::dump()
{
    m_selector->addRule(m_targetOpCode, m_rule);
    m_targetOpCode = MirInstructionOpCode::INVALID;
}
