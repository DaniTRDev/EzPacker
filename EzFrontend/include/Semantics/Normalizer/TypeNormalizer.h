#ifndef EZPACKER_TYPENORMALIZER_H
#define EZPACKER_TYPENORMALIZER_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Parser/Ast/IdentifierNode.h"
#include "Parser/Ast/TokenTypeNode.h"

class TypeNormalizer : public INormalizer<TypeEntry>
{
  public:
    /**
     * Tries to normalize the given Type.
     * @param node
     * @param context
     * @return std::shared_ptr<TypeEntry>
     */
    std::shared_ptr<TypeEntry> normalizeNode(std::shared_ptr<Ast> node,
                                             const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_TYPENORMALIZER_H
