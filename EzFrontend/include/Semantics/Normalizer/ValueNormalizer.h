#ifndef EZPACKER_VALUENORMALIZER_H
#define EZPACKER_VALUENORMALIZER_H

#include "EzFrontendCommon.h"
#include "Parser/Ast/ValueNode.h"
#include "Semantics/Normalizer/INormalizer.h"

struct NormalizedValue
{
};

class ValueNormalizer : public INormalizer<NormalizedValue>
{
  public:
    /**
     * Tries to normalize given value node.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedResult>
     */
    std::shared_ptr<NormalizedValue> normalizeNode(std::shared_ptr<Ast> node,
                                                   const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_VALUENORMALIZER_H
