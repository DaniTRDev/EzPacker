#include "Semantics/Normalizer/MemoryNodeNormalizer.h"

std::shared_ptr<NormalizedMemoryNode> MemoryNodeNormalizer::normalizeNode(
    std::shared_ptr<Ast> node, const std::shared_ptr<NormalizerContext> &context)
{
    if (!node || node->getType() != AstType::Memory)
    {
        // TODO: Given node is not a memory node
        return nullptr;
    }

    auto memoryNode = Ast::cast<MemoryNode>(node);
    std::shared_ptr<NormalizedMemoryNode> normalizedMemoryNode = std::make_shared<NormalizedMemoryNode>();

    switch (memoryNode->getMemRefType())
    {
    case IRMemoryReferenceType::Direct: {
        normalizedMemoryNode->m_absolute =
            ValueNormalizer().normalizeNode(memoryNode->castChildTo<ValueNode>(0), context);
        break;
    }
    case IRMemoryReferenceType::Base: {
        normalizedMemoryNode->m_base =
            VirtualVariableNormalizer().normalizeNode(memoryNode->castChildTo<VirtualVariableNode>(0), context);
        break;
    }
    case IRMemoryReferenceType::BaseDisplacement: {
        normalizedMemoryNode->m_base =
            VirtualVariableNormalizer().normalizeNode(memoryNode->castChildTo<VirtualVariableNode>(0), context);
        normalizedMemoryNode->m_displ = ValueNormalizer().normalizeNode(memoryNode->castChildTo<ValueNode>(1), context);
        break;
    }
    case IRMemoryReferenceType::IndexScale: {
        normalizedMemoryNode->m_index =
            VirtualVariableNormalizer().normalizeNode(memoryNode->castChildTo<VirtualVariableNode>(0), context);
        normalizedMemoryNode->m_scale = ValueNormalizer().normalizeNode(memoryNode->castChildTo<ValueNode>(1), context);
        break;
    }
    case IRMemoryReferenceType::BaseIndexScaleDisplacement: {
        normalizedMemoryNode->m_base =
            VirtualVariableNormalizer().normalizeNode(memoryNode->castChildTo<VirtualVariableNode>(0), context);
        normalizedMemoryNode->m_index =
            VirtualVariableNormalizer().normalizeNode(memoryNode->castChildTo<VirtualVariableNode>(1), context);
        normalizedMemoryNode->m_scale = ValueNormalizer().normalizeNode(memoryNode->castChildTo<ValueNode>(2), context);
        normalizedMemoryNode->m_displ = ValueNormalizer().normalizeNode(memoryNode->castChildTo<ValueNode>(3), context);
        break;
    }
    case IRMemoryReferenceType::IPRelative: {
        // TODO: Implement.
        break;
    }
    default: {
        // TODO: Invalid memory reference type.
        return nullptr;
    }
    }

    if (normalizedMemoryNode->m_base == nullptr && normalizedMemoryNode->m_index == nullptr &&
        normalizedMemoryNode->m_scale == nullptr && normalizedMemoryNode->m_displ == nullptr &&
        normalizedMemoryNode->m_absolute)
    {
        // TODO: Expected at least 1 argument to conform a memory reference.
        return nullptr;
    }
    
    return normalizedMemoryNode;
}
