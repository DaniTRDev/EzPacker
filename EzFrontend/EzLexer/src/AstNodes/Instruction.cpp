#include "AstNodes/Instruction.h"

Instruction::Instruction(std::string instructionName, std::vector<std::shared_ptr<AstNode>> operands) :
    m_instructionName(instructionName), m_operands(operands)
{
}

AstNodeType Instruction::getType() const { return AstNodeType::Instruction; }

const char *Instruction::getAstNodeName() const { return "Instruction"; }

const std::string &Instruction::getInstructionName() const { return m_instructionName; }

size_t Instruction::getOperandCount() const { return m_operands.size(); }

const std::vector<std::shared_ptr<AstNode>> &Instruction::getOperands() const { return m_operands; }

std::string Instruction::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Instruction(name: {}) {{\n", m_instructionName);

    if (mode == AstNodeStringMode::Debug)
    {
        auto &operands = getOperands();
        for (size_t i = 0; i < operands.size(); i++)
        {
            res += std::format("\t{} = {}\n", i, operands[i]->getAsStr(mode));
        }
    }

    res += "}\n";
    return std::move(res);
}

CallInstruction::CallInstruction(std::string calleeName,
                                 std::string returnType,
                                 std::vector<std::shared_ptr<AstNode>> parameters) :
    m_calleeName(std::move(calleeName)), m_returnType(std::move(returnType)), Instruction("call", std::move(parameters))
{
}

const std::string &CallInstruction::getCalleeName() const { return m_calleeName; }

const std::string &CallInstruction::getReturnType() const { return m_returnType; }

std::string CallInstruction::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Call(type: {} name: {}) {{\n", m_returnType, m_calleeName);

    if (mode == AstNodeStringMode::Debug)
    {
        auto &parameters = getOperands();
        for (size_t i = 0; i < parameters.size(); i++)
        {
            res += std::format("{} = {}\n", i, parameters[i]->getAsStr(mode));
        }
    }

    res += "}\n";
    return std::move(res);
}
