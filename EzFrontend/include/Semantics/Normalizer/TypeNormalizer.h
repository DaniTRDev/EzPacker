#ifndef EZPACKER_TYPENORMALIZER_H
#define EZPACKER_TYPENORMALIZER_H

#include "EzFrontendCommon.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Parser/Ast/IdentifierNode.h"
#include "Parser/Ast/TokenTypeNode.h"

struct NormalizedType
{
    std::shared_ptr<SourceReference> m_sourceRef; // Source reference of this type.
    std::shared_ptr<TypeEntry> m_typeEntry; // Entry of the type table.
};

class TypeNormalizer : public INormalizer<NormalizedType>
{
  public:
    /**
     * Tries to normalize the given Type. Assumes given node is valid and is of type AstNode::Type.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedType>
     */
    std::shared_ptr<NormalizedType> normalizeNode(std::shared_ptr<Ast> node,
                                             const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_TYPENORMALIZER_H
