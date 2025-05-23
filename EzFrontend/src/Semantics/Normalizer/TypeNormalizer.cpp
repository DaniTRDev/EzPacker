#include "Semantics/Normalizer/TypeNormalizer.h"

std::shared_ptr<TypeEntry> TypeNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                         const std::shared_ptr<NormalizerContext> &context)
{
    if (!node || node->getType() != AstType::Type)
    {
        // TODO: Given node is not a type node.
        return nullptr;
    }

    auto typeIdentifier = node->castChildTo<IdentifierNode>(0);
    if (!typeIdentifier)
    {
        // TODO: "Given type is malformed"
        return nullptr;
    }
    
    auto tokenTypeNode = node->castChildTo<TokenTypeNode>(0);
    if (!tokenTypeNode)
    {
        // TODO: "Given type doesn't have valid content"
        return nullptr;
    }
    
    auto &typeTable = context->m_typeTable;
    std::string typeName = tokenTypeNode->getContent();
    
    if (!typeTable->doesElemExist(typeName))
    {
        // TODO: "Undefined type"
        return nullptr;
    }
    
    return typeTable->getElement(typeName);
}