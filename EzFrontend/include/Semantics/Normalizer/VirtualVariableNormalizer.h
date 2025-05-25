#ifndef EZPACKER_VIRTUALVARIABLENORMALIZER_H
#define EZPACKER_VIRTUALVARIABLENORMALIZER_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/SymbolNormalizer.h"
#include "Semantics/Normalizer/TypeNormalizer.h"

struct NormalizedVirtualVariable
{
    std::shared_ptr<NormalizedSymbol> m_symbol;
    std::shared_ptr<NormalizedType> m_type;
};

class VirtualVariableNormalizer : public INormalizer<NormalizedVirtualVariable>
{
  public:
    /**
     * Tries to normalize the given virtual variable. This only creates the virtual variable, type must be obtained
     * with deeper semantics (type evaluator). Assumes node is valid and it's type is AstType::VirtualVariable.
     * @param node
     * @param context
     * @return std::shared_ptr<SymbolEntry>
     */
    std::shared_ptr<NormalizedVirtualVariable> normalizeNode(
        std::shared_ptr<Ast> node, const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_VIRTUALVARIABLENORMALIZER_H
