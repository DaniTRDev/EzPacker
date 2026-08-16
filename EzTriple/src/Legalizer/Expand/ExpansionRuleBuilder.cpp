#include "Legalizer/Expand/ExpansionRuleBuilder.h"

ExpansionRuleBuilder::ExpansionRuleBuilder(MirExpansionRuleRegistry *registry, MirInstructionOpCode target) :
    m_registry(registry), m_target(target), m_rule(ExpansionRule(registry->getAllocator()))
{
}

ExpansionRuleBuilder::~ExpansionRuleBuilder() { dump(); }

ExpansionRuleBuilder &ExpansionRuleBuilder::emit(MirInstructionOpCode opcode, std::vector<ExpansionOperand> operands)
{
    auto instr = ExpansionInstruction{ .m_opcode = opcode,
                                       .m_rtLibraryCall = "",
                                       .m_operands = std::pmr::vector<ExpansionOperand>(m_registry->getAllocator()) };
    instr.m_operands.insert(instr.m_operands.begin(), operands.begin(), operands.end());
    m_rule.m_instructions.push_back(std::move(instr));

    return *this;
}

ExpansionRuleBuilder &ExpansionRuleBuilder::emit(const std::string_view &rtSymbol,
                                                 std::vector<ExpansionOperand> operands)
{
    auto instr = ExpansionInstruction{ .m_opcode = MirInstructionOpCode::CALL,
                                       .m_rtLibraryCall = std::pmr::string(rtSymbol, m_registry->getAllocator()),
                                       .m_operands = std::pmr::vector<ExpansionOperand>(m_registry->getAllocator()) };
    instr.m_operands.insert(instr.m_operands.begin(), operands.begin(), operands.end());
    m_rule.m_instructions.push_back(std::move(instr));

    return *this;
}

ExpansionRuleBuilder &ExpansionRuleBuilder::pred(ExpansionPredicate pred)
{
    m_rule.m_pred = std::move(pred);
    return *this;
}

void ExpansionRuleBuilder::dump()
{
    m_registry->addRule(m_target, std::move(m_rule));

    m_target = MirInstructionOpCode::INVALID;
    m_rule = ExpansionRule(m_registry->getAllocator());
}