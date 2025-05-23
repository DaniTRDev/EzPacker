#ifndef EZPACKER_NORMALIZEDMEMORYNODE_H
#define EZPACKER_NORMALIZEDMEMORYNODE_H

#include "EzFrontendCommon.h"
#include "Parser/Ast/MemoryNode.h"
#include "Semantics/Normalizer/INormalizer.h"
#include "Semantics/Normalizer/VirtualVariableNormalizer.h"
#include "Semantics/Normalizer/ValueNormalizer.h"

struct NormalizedMemoryNode
{
    std::shared_ptr<NormalizedVirtualVariable> m_base;
    std::shared_ptr<NormalizedVirtualVariable> m_index;
    
    std::shared_ptr<NormalizedValue> m_scale;
    std::shared_ptr<NormalizedValue> m_displ;
    std::shared_ptr<NormalizedValue> m_absolute; // Absolute address.
};

class MemoryNodeNormalizer : public INormalizer<NormalizedMemoryNode>
{
  public:
    /**
     * Tries to normalize the given memory node.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedMemoryNode>
     */
    std::shared_ptr<NormalizedMemoryNode> normalizeNode(std::shared_ptr<Ast> node,
                                                        const std::shared_ptr<NormalizerContext> &context) override;
};

#endif // EZPACKER_NORMALIZEDMEMORYNODE_H
