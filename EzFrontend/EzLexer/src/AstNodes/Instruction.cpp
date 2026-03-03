#include "AstNodes/Instruction.h"

Instruction::Instruction(TypedPoolSlice<AstNode> *operands, std::string_view instructionName) :
    m_instructionName(instructionName)
{
    AstNodeContainer::setExpressions(operands);
}

AstNodeType Instruction::getType() const { return AstNodeType::Instruction; }

bool Instruction::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *Instruction::getAstNodeName() const { return "Instruction"; }

const std::string_view &Instruction::getInstructionName() const { return m_instructionName; }

std::string Instruction::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Instruction(name: {}) {{\n", m_instructionName);

    if (mode == AstNodeStringMode::Debug)
    {
        auto operand = getExpressions()->m_head;
        size_t i = 0;

        while (operand && operand->m_object)
        {
            AstNode *node = (AstNode *)operand->m_object;
            res += std::format("\t{} = {}\n", i, node->getAsStr(mode));
            operand = operand->m_next;
            i++;
        }
    }

    res += "}\n";
    return std::move(res);
}

CallInstruction::CallInstruction(TypedPoolSlice<AstNode> *params, std::string_view calleeName, std::string_view returnType) :
    m_calleeName(std::move(calleeName)), m_returnType(std::move(returnType)), Instruction(params, "call")
{
}

const std::string_view &CallInstruction::getCalleeName() const { return m_calleeName; }

const std::string_view &CallInstruction::getReturnType() const { return m_returnType; }

std::string CallInstruction::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Call(type: {} name: {}) {{\n", m_returnType, m_calleeName);

    if (mode == AstNodeStringMode::Debug)
    {
        auto parameter = getExpressions()->m_head;
        size_t i = 0;

        while (parameter && parameter->m_object)
        {
            AstNode *node = (AstNode *)parameter->m_object;
            res += std::format("\t{} = {}\n", i, node->getAsStr(mode));
            parameter = parameter->m_next;
            i++;
        }
    }

    res += "}\n";
    return std::move(res);
}
