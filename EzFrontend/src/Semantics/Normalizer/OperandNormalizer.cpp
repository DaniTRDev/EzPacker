#include "Semantics/Normalizer/OperandNormalizer.h"

std::shared_ptr<NormalizedOperand> OperandNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                                    const std::shared_ptr<NormalizerContext> &context)
{
    std::shared_ptr<NormalizedOperand> operand = std::make_shared<NormalizedOperand>();

    if (node->getType() == AstType::Memory)
    {
        operand->m_operandType = NormalizedOperandType::Memory;
        operand->m_memoryNode = MemoryNodeNormalizer().normalizeNode(node, context);
        if (operand->m_memoryNode == nullptr)
        {
            context->m_logger->logError(LogMessage("").add("Error normalizing memory operand"), node->getSourceRef());
            return nullptr;
        }
    }
    else if (node->getType() == AstType::VirtualVariable)
    {
        operand->m_operandType = NormalizedOperandType::VirtualVariable;
        operand->m_virtualVariable = VirtualVariableNormalizer().normalizeNode(node, context);
        if (operand->m_virtualVariable == nullptr)
        {
            context->m_logger->logError(LogMessage("").add("Error normalizing virtual variable operand"),
                                        node->getSourceRef());
            return nullptr;
        }
    }
    else if (node->getType() == AstType::Value)
    {
        operand->m_operandType = NormalizedOperandType::Value;
        operand->m_value = ValueNormalizer().normalizeNode(node, context);
        if (operand->m_value == nullptr)
        {
            context->m_logger->logError(LogMessage("").add("Error normalizing value"), node->getSourceRef());
            return nullptr;
        }
    }
    else
    {
        context->m_logger->logError(LogMessage("").add("Given node can't be normalized into operand"),
                                    node->getSourceRef());
        return nullptr;
    }

    return operand;
}
