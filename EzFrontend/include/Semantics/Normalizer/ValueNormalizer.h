#ifndef EZPACKER_VALUENORMALIZER_H
#define EZPACKER_VALUENORMALIZER_H

#include "EzFrontendCommon.h"
#include "Number/BigInt.h"
#include "Number/Double.h"
#include "Number/Float.h"
#include "Parser/Ast/TokenTypeNode.h"
#include "Parser/Ast/ValueNode.h"
#include "Semantics/Normalizer/INormalizer.h"

enum class NormalizedValueType : uint8_t
{
    Invalid = 0,
    String,
    Number // Int, FloatingPoint, Double, ...
};

struct NormalizedValue
{
    NormalizedValueType m_type;
    std::shared_ptr<SourceReference> m_sourceRef;
    std::shared_ptr<IBigNumber> m_number;
    std::string m_string; // If value is a string.
};

class ValueNormalizer : public INormalizer<NormalizedValue>
{
  public:
    /**
     * Tries to normalize given value node. Assumes given node is valid an is of type AstType::Value
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedResult>
     */
    std::shared_ptr<NormalizedValue> normalizeNode(std::shared_ptr<Ast> node,
                                                   const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_VALUENORMALIZER_H
