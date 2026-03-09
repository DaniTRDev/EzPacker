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

CallInstruction::CallInstruction(TypedPoolSlice<AstNode> *params) : Instruction(params, "call") {}