#include "Semantics/Normalizer/SymbolNormalizer.h"

std::shared_ptr<NormalizedSymbol> SymbolNormalizer::normalizeNode(std::shared_ptr<Ast> node,
                                                                  const std::shared_ptr<NormalizerContext> &context)
{
    // Caller must set symbol's origin.

    auto tokenTypeNode = node->castChildTo<TokenTypeNode>(0);
    auto &symbolTable = context->m_symbolTable;
    std::string symbolName = tokenTypeNode->getContent();

    /*if (symbolTable->doesElemExist(symbolName))
    {
        context->m_logger->logError(LogMessage("").add("Redefinition of symbol {}", symbolName),
                                    tokenTypeNode->getSourceRef());
        return nullptr;
    }*/

    std::shared_ptr<SymbolEntry> symbolEntry = std::make_shared<SymbolEntry>();
    std::shared_ptr<NormalizedSymbol> normalizedSymbol = std::make_shared<NormalizedSymbol>();

    symbolEntry->m_name = symbolName;
    normalizedSymbol->m_symbolEntry = symbolEntry;
    normalizedSymbol->m_sourceRef = tokenTypeNode->getSourceRef();

    symbolTable->addElement(symbolEntry, symbolName);
    return std::move(normalizedSymbol);
}