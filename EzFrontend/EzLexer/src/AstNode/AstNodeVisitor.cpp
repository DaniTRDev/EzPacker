#include "AstNode/AstNodeVisitor.h"

bool AstNodeVisitor::visitBaseClass(const std::shared_ptr<struct AstNode> &astNode)
{
    switch (astNode->getType())
    {
        case AstNodeType::Invalid:
        {
            return false;
        }
        case AstNodeType::Instruction:
        {
            return visit(std::dynamic_pointer_cast<Instruction>(astNode));
        };
        case AstNodeType::Immediate:
        {
            return visit(std::dynamic_pointer_cast<ImmediateOperand>(astNode));
        };
        case AstNodeType::Label:
        {
            return visit(std::dynamic_pointer_cast<Label>(astNode));
        };
        case AstNodeType::MemoryOperand:
        {
            return visit(std::dynamic_pointer_cast<MemoryOperandAstNode>(astNode));
        };
        case AstNodeType::Module:
        {
            return visit(std::dynamic_pointer_cast<Module>(astNode));
        };
        case AstNodeType::Variable:
        {
            return visit(std::dynamic_pointer_cast<Variable>(astNode));
        };
        default:
        {
            return false;
        }
    }
    return false;
}
