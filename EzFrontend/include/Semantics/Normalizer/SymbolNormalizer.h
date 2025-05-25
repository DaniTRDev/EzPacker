#ifndef EZPACKER_SYMBOLNORMALIZER_H
#define EZPACKER_SYMBOLNORMALIZER_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Parser/Ast/TokenTypeNode.h"

struct NormalizedSymbol
{
    std::shared_ptr<SourceReference> m_sourceRef;
    std::shared_ptr<SymbolEntry> m_symbolEntry; // Entry in the symbol table.
};

class SymbolNormalizer : public INormalizer<NormalizedSymbol>
{
  public:
    /**
     * Tries to normalize the given Symbol. This only adds an entry to the symbol table. Caller MUST set the symbol
     * origin (SymbolType). Assumes given node is valid and its type is AstType::Identifier.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedSymbol>
     */
    std::shared_ptr<NormalizedSymbol> normalizeNode(std::shared_ptr<Ast> node,
                                               const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_SYMBOLNORMALIZER_H
