#ifndef EZPACKER_SYMBOLNORMALIZER_H
#define EZPACKER_SYMBOLNORMALIZER_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Parser/Ast/TokenTypeNode.h"

class SymbolNormalizer : public INormalizer<SymbolEntry>
{
  public:
    /**
     * Tries to normalize the given Symbol. This only adds an entry to the symbol table. Caller MUST set the symbol
     * origin (SymbolType).
     * @param node
     * @param context
     * @return std::shared_ptr<SymbolEntry>
     */
    std::shared_ptr<SymbolEntry> normalizeNode(std::shared_ptr<Ast> node,
                                               const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_SYMBOLNORMALIZER_H
