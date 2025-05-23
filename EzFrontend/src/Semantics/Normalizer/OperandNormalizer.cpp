#include "Semantics/Normalizer/OperandNormalizer.h"

std::shared_ptr<NormalizedOperand> OperandNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                                    const std::shared_ptr<NormalizerContext> &context)
{
    if (!node)
    {
        // TODO: Given node is not a valid operand node.
        return nullptr;
    }

    std::shared_ptr<NormalizedOperand> operand = std::make_shared<NormalizedOperand>();

    if (node->getType() == AstType::Memory)
    {
        operand->m_operandType = NormalizedOperandType::Memory;
        operand->m_memoryNode = MemoryNodeNormalizer().normalizeNode(node, context);
        if (operand->m_memoryNode == nullptr)
        {
            // TODO: "Could not normalize given memory"
            return nullptr;
        }
    }
    else if (node->getType() == AstType::VirtualVariable)
    {
        operand->m_operandType = NormalizedOperandType::VirtualVariable;
        operand->m_virtualVariable = VirtualVariableNormalizer().normalizeNode(node, context);
        if (operand->m_virtualVariable == nullptr)
        {
            // TODO: "Could not normalize given virtual variable"
            return nullptr;
        }
    }
    else if (node->getType() == AstType::Value)
    {
        operand->m_operandType = NormalizedOperandType::Value;
        operand->m_value = ValueNormalizer().normalizeNode(node, context);
        if (operand->m_virtualVariable == nullptr)
        {
            // TODO: "Could not normalize given virtual variable"
            return nullptr;
        }
    }
    else
    {
        // TODO: "Given node can't be normalized into an operand."
        return nullptr;
    }

    return operand;
}
