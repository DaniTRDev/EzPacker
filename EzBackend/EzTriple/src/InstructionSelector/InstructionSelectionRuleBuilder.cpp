#include "InstructionSelector/InstructionSelectionRuleBuilder.h"

InstructionSelectionRuleBuilder::InstructionSelectionRuleBuilder(MirInstructionSelector *selector) :
    m_rule(InstructionSelectionRule{
            .m_name = "", .m_pred = {}, .m_actions = std::pmr::list<InstructionSelAction>(selector->getAlloc()) }),
    m_targetOpCode(MirInstructionOpCode::INVALID), m_selector(selector)
{
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::act(const InstructionSelAction &act)
{
    m_rule.m_actions.push_back(act);
    return *this;
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::begin(const char *name, MirInstructionOpCode opcode)
{
    m_targetOpCode = opcode;
    m_rule = InstructionSelectionRule{ .m_name = name,
                                       .m_pred = {},
                                       .m_actions = std::pmr::list<InstructionSelAction>(m_selector->getAlloc()) };
    return *this;
}

InstructionSelectionRuleBuilder &InstructionSelectionRuleBuilder::pred(const InstructionSelPred &pred)
{
    m_rule.m_pred = pred;
    return *this;
}

void InstructionSelectionRuleBuilder::dump()
{
    m_selector->addRule(m_targetOpCode, std::move(m_rule));
    m_targetOpCode = MirInstructionOpCode::INVALID;
}
