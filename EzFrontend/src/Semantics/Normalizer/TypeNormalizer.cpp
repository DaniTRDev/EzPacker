#include "Semantics/Normalizer/TypeNormalizer.h"

std::shared_ptr<NormalizedType> TypeNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                              const std::shared_ptr<NormalizerContext> &context)
{
    auto typeIdentifier = node->castChildTo<IdentifierNode>(0);
    auto tokenTypeNode = typeIdentifier->castChildTo<TokenTypeNode>(0);
    if (!tokenTypeNode)
    {
        context->m_logger->logError(LogMessage("").add("Given type is not valid"), tokenTypeNode->getSourceRef());
        return nullptr;
    }

    auto &typeTable = context->m_typeTable;
    std::string typeName = tokenTypeNode->getContent();

    if (!typeTable->doesElemExist(typeName))
    {
        context->m_logger->logError(LogMessage("").add("Undefined type {}", typeName), tokenTypeNode->getSourceRef());
        return nullptr;
    }

    std::shared_ptr<NormalizedType> normalizedType = std::make_shared<NormalizedType>();
    normalizedType->m_typeEntry = typeTable->getElement(typeName);
    normalizedType->m_sourceRef = tokenTypeNode->getSourceRef();

    return std::move(normalizedType);
}