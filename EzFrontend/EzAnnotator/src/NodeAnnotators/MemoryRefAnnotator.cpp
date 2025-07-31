#include "NodeAnnotators/MemoryRefAnnotator.h"

MemoryRefAnnotator::MemoryRefAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable)
{
    setSymbolTable(symbolTable);
}

bool MemoryRefAnnotator::annotate(const std::shared_ptr<AstNode> &node,
                                  const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid memory ref node"));
        return false;
    }

    MemoryReferenceType type = MemoryReferenceType::Invalid;
    size_t baseId = 0, indexId = 0;
    std::shared_ptr<ConstantAnnotation> displ = nullptr, scale = nullptr;
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    auto vvAnnotator = std::make_shared<VirtualVariableAnnotator>(getSymbolTable());

    /**
     * We can safely use a constant annotator here. We have guaranteed (thanks to the parser) that the given
     * node is well-formed and an memory reference WILL NOT contain strings or floats.
     */

    if (node->getId() == AstNodes::DirectMemory().getId())
    {
        type = MemoryReferenceType::Direct;
        if (!ccAnnotator->annotate(node->getChild(0), logger))
        {
            logger->logSourceError(LogMessage("Invalid memory address"), sourceRef);
            return false;
        }

        displ = std::dynamic_pointer_cast<ConstantAnnotation>(node->getChild(0)->getAnnotation());
        node->removeChild();
    }
    else
    {
        // All the references below start with a virtual variable, annotate it and then check.
        if (!vvAnnotator->annotate(node->getChild(0), logger))
        {
            logger->logSourceError(LogMessage("Invalid base/index"), sourceRef);
            return false;
        }

        if (node->getId() == AstNodes::BaseMemory().getId())
        {
            type = MemoryReferenceType::Base;
            baseId = std::dynamic_pointer_cast<SymbolAnnotation>(node->getChild(0)->getAnnotation())->getSymbolId();
        }
        else if (node->getId() == AstNodes::BaseDisplMemory().getId())
        {
            if (!ccAnnotator->annotate(node->getChild(1), logger))
            {
                logger->logSourceError(LogMessage("Invalid displacement"), sourceRef);
                return false;
            }

            type = MemoryReferenceType::BaseDispl;
            baseId = std::dynamic_pointer_cast<SymbolAnnotation>(node->getChild(0)->getAnnotation())->getSymbolId();
            displ = std::dynamic_pointer_cast<ConstantAnnotation>(node->getChild(1)->getAnnotation());

            node->removeChild(); // Displ
        }
        else if (node->getId() == AstNodes::BaseIndexScaleDisplMemory().getId())
        {
            if (!vvAnnotator->annotate(node->getChild(1), logger))
            {
                logger->logSourceError(LogMessage("Invalid index"), sourceRef);
                return false;
            }

            if (!ccAnnotator->annotate(node->getChild(2), logger))
            {
                logger->logSourceError(LogMessage("Invalid scale"), sourceRef);
                return false;
            }

            if (!ccAnnotator->annotate(node->getChild(3), logger))
            {
                logger->logSourceError(LogMessage("Invalid displ"), sourceRef);
                return false;
            }

            type = MemoryReferenceType::BaseIndexScaleDispl;
            baseId = std::dynamic_pointer_cast<SymbolAnnotation>(node->getChild(0)->getAnnotation())->getSymbolId();
            indexId = std::dynamic_pointer_cast<SymbolAnnotation>(node->getChild(1)->getAnnotation())->getSymbolId();
            scale = std::dynamic_pointer_cast<ConstantAnnotation>(node->getChild(2)->getAnnotation());
            displ = std::dynamic_pointer_cast<ConstantAnnotation>(node->getChild(3)->getAnnotation());

            node->removeChild(); // Index
            node->removeChild(); // Scale
            node->removeChild(); // Displ
        }
        else if (node->getId() == AstNodes::IndexScaleMemory().getId())
        {
            if (!ccAnnotator->annotate(node->getChild(1), logger))
            {
                logger->logSourceError(LogMessage("Invalid scale"), sourceRef);
                return false;
            }

            type = MemoryReferenceType::IndexScale;
            indexId = std::dynamic_pointer_cast<SymbolAnnotation>(node->getChild(0)->getAnnotation())->getSymbolId();
            scale = std::dynamic_pointer_cast<ConstantAnnotation>(node->getChild(1)->getAnnotation());

            node->removeChild(1); // Scale
        }

        node->removeChild(); // Base / Index
    }

    node->setAnnotation(std::make_shared<MemoryRefAnnotation>(type, baseId, indexId, m_refTypeId, displ, scale));
    return true;
}

bool MemoryRefAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::BaseMemory().getId() || node->getId() == AstNodes::BaseDisplMemory().getId() ||
            node->getId() == AstNodes::BaseIndexScaleDisplMemory().getId() ||
            node->getId() == AstNodes::DirectMemory().getId() || node->getId() == AstNodes::IndexScaleMemory().getId();
}

void MemoryRefAnnotator::setMemoryType(size_t typeId) { m_refTypeId = typeId; }
