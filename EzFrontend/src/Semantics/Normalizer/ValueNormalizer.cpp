#include "Semantics/Normalizer/ValueNormalizer.h"

std::shared_ptr<NormalizedValue> ValueNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                                const std::shared_ptr<NormalizerContext> &context)
{
    auto valueNode = Ast::cast<ValueNode>(node);
    auto tokenTypeNode = node->castChildTo<TokenTypeNode>(0);
    if (!tokenTypeNode)
    {
        context->m_logger->logError(LogMessage("").add("Given value is not valid"), tokenTypeNode->getSourceRef());
        return nullptr;
    }

    std::shared_ptr<NormalizedValue> normalizedValue = std::make_shared<NormalizedValue>();
    std::string valueContentStr = tokenTypeNode->getContent();

    switch (valueNode->getValueType())
    {
    case ValueNodeType::FloatingPoint: {
        std::shared_ptr<IBigNumber> number = std::make_shared<Float>();
        if (!number->fromStr(valueContentStr))
        {
            // Try to cast to a double.
            number = std::make_shared<Double>();
            if (!number->fromStr(valueContentStr))
            {
                context->m_logger->logError(
                    LogMessage("").add("Given floating point value could not be converted to bits"),
                    tokenTypeNode->getSourceRef());
                return nullptr;
            }
        }

        normalizedValue->m_type = NormalizedValueType::Number;
        normalizedValue->m_number = std::move(number);
        break;
    }
    case ValueNodeType::Int: {
        std::shared_ptr<IBigNumber> number = std::make_shared<BigInt>();
        if (!number->fromStr(valueContentStr))
        {
            context->m_logger->logError(LogMessage("").add("Given integer value could not be converted to bits"),
                                        tokenTypeNode->getSourceRef());
            return nullptr;
        }

        normalizedValue->m_type = NormalizedValueType::Number;
        normalizedValue->m_number = std::move(number);
        break;
    }
    case ValueNodeType::String: {
        normalizedValue->m_type = NormalizedValueType::String;
        normalizedValue->m_string = valueContentStr;
        break;
    }
    default: {
        context->m_logger->logError(LogMessage("").add("Given value doesn't have a valid type"),
                                    tokenTypeNode->getSourceRef());
        return nullptr;
    }
    }

    normalizedValue->m_sourceRef = tokenTypeNode->getSourceRef();
    return std::move(normalizedValue);
}
