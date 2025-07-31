#include "NodeAnnotators/VirtualVariableAnnotator.h"

VirtualVariableAnnotator::VirtualVariableAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable) :
    m_typeId(0), m_workingMode(VVAnnotatorWorkingMode::ExpectsExistingSymbol)
{
    setSymbolTable(symbolTable);
}

bool VirtualVariableAnnotator::annotate(const std::shared_ptr<AstNode> &node,
                                        const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If there's no logger, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid virtual variable node"));
        return false;
    }

    const std::shared_ptr<AstNode> &identifierNode = node->getChild(0);
    std::shared_ptr<SymbolAnnotator> ssAnnotator = std::make_shared<SymbolAnnotator>(getSymbolTable());

    if (m_workingMode == VVAnnotatorWorkingMode::ExpectsExistingSymbol)
        ssAnnotator->setWorkingMode(SymbolAnnotatorWorkingMode::ExpectsExistingSymbol);
    else
        ssAnnotator->setWorkingMode(SymbolAnnotatorWorkingMode::CreateNewSymbol);

    ssAnnotator->setTypeId(m_typeId);
    if (!ssAnnotator->annotate(identifierNode, logger))
    {
        logger->logSourceError(LogMessage("Invalid virtual variable symbol"), node->getSourceRef());
        return false;
    }

    node->setAnnotation(identifierNode->getAnnotation()); // Move the annotation to the VV node itself.
    node->removeChild();

    return true;
}

bool VirtualVariableAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::VirtualVariable().getId();
}

void VirtualVariableAnnotator::setTypeId(size_t typeId) { m_typeId = typeId; }

void VirtualVariableAnnotator::setWorkingMode(VVAnnotatorWorkingMode mode) { m_workingMode = mode; }
