#include "Semantics/Normalizer/SymbolNormalizer.h"

std::shared_ptr<SymbolEntry> SymbolNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                             const std::shared_ptr<NormalizerContext> &context)
{
    // Caller must set symbol's origin.
    if (!node || node->getType() != AstType::Identifier)
    {
        // TODO: Given node is not a symbol node.
        return nullptr;
    }

    auto tokenTypeNode = node->castChildTo<TokenTypeNode>(0);
    if (!tokenTypeNode)
    {
        // TODO: "Given type doesn't have valid content"
        return nullptr;
    }

    auto &symbolTable = context->m_symbolTable;
    std::string symbolName = tokenTypeNode->getContent();

    if (symbolTable->doesElemExist(symbolName))
    {
        // TOOD: Redefined symbol
        return nullptr;
    }

    std::shared_ptr<SymbolEntry> symbol = std::make_shared<SymbolEntry>();
    symbol->m_name = symbolName;
    symbolTable->addElement(symbol, symbolName);

    return symbol;
}