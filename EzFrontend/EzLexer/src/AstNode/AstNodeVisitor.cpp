#include "AstNode/AstNodeVisitor.h"

/*
 * TODO:
 * AFTER creating the typed pool. And implementing new cache-friendly data structures.
 *
 * Remove this and use a templated visitor "accept".
 */
bool AstNodeVisitor::visitBaseClass(const std::shared_ptr<struct AstNode> &astNode)
{
    switch (astNode->getType())
    {
        case AstNodeType::Invalid:
        {
            return false;
        }
        case AstNodeType::If:
        {
            return visit(std::dynamic_pointer_cast<IfAstNode>(astNode));
        }
        case AstNodeType::While:
        {
            return visit(std::dynamic_pointer_cast<Instruction>(astNode));
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

bool AstNodeVisitor::visitAll(const std::vector<std::shared_ptr<struct AstNode>> &nodes)
{
    for (const auto &node : nodes)
    {
        if (!visitBaseClass(node))
        {
            return false;
        }
    }
    return true;
}
