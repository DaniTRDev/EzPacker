#include "Semantics/Normalizer/InstructionNormalizer.h"

std::shared_ptr<NormalizedInstruction> InstructionNormalizer::normalizeNode(
    std::shared_ptr<Ast> node, const std::shared_ptr<NormalizerContext> &context)
{
    auto instructionNode = Ast::cast<InstructionNode>(node);
    auto instructionNameIdentifierNode = instructionNode->castChildTo<IdentifierNode>(0);
    if (!instructionNameIdentifierNode)
    {
        context->m_logger->logError(LogMessage("").add("Given instruction identifier is not valid"),
                                    node->getSourceRef());
        return nullptr;
    }

    auto instructionNameTokenNode = instructionNameIdentifierNode->castChildTo<TokenTypeNode>(0);
    if (!instructionNameTokenNode)
    {
        context->m_logger->logError(LogMessage("").add("Given instruction name token is not valid"),
                                    node->getSourceRef());
        return nullptr;
    }

    std::string instructionName = instructionNameTokenNode->getContent();
    if (!context->m_instructionTable->doesElemExist(instructionName))
    {
        context->m_logger->logError(
            LogMessage("").add("Given instruction name does not represent any known instruction"),
            node->getSourceRef());
        return nullptr;
    }

    auto &children = node->getChildren();
    bool expectsOperand = false;
    std::shared_ptr<NormalizedInstruction> normalizedInstruction = std::make_shared<NormalizedInstruction>();
    std::shared_ptr<NormalizedOperand> operand;
    std::shared_ptr<NormalizedType> operandType;

    for (size_t i = 1; i < children.size(); i++)
    {
        // Operand syntaxis: type operand
        if (i % 2 == 1)
        {
            operandType = TypeNormalizer().normalizeNode(children[i], context);
            if (!operandType)
            {
                context->m_logger->logError(LogMessage("").add("Invalid operand type"), children[i]->getSourceRef());
                return nullptr;
            }

            expectsOperand = true;
        }
        else
        {
            operand = OperandNormalizer().normalizeNode(children[i], context);
            if (!operand)
            {
                context->m_logger->logError(LogMessage("").add("Invalid operand"), children[i]->getSourceRef());
                return nullptr;
            }

            expectsOperand = false;
            operand->m_dataType = std::move(operandType);
            normalizedInstruction->m_operands.push_back(std::move(operand));
        }
    }

    if (expectsOperand)
    {
        context->m_logger->logError(LogMessage("").add("Expected operand after type"), node->getSourceRef());
        return nullptr;
    }

    return std::move(normalizedInstruction);
}
