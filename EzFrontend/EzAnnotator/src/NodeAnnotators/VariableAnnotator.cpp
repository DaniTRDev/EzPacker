#include "NodeAnnotators/VariableAnnotator.h"

VariableAnnotator::VariableAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                                     const ScopeAbleAnnotator::TypeTableT &typeTable)
{
    setSymbolTable(symbolTable);
    setTypeTable(typeTable);
}

bool VariableAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit right away.

    if (!node)
    {
        logger->logError(LogMessage("Invalid variable node"));
        return false;
    }

    const std::shared_ptr<AstNode> &variableName = node->getChild(0);
    const std::shared_ptr<AstNode> &variableType = node->getChild(1);
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    std::shared_ptr<SymbolAnnotator> ssAnnotator = std::make_shared<SymbolAnnotator>(getSymbolTable());
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(getTypeTable());

    ssAnnotator->setWorkingMode(SymbolAnnotatorWorkingMode::CreateNewSymbol);
    if (!ssAnnotator->annotate(variableName, logger))
    {
        logger->logSourceError(LogMessage("Invalid variable name {}", variableName->getContent()),
                               variableName->getSourceRef());
        return false;
    }

    if (!ttAnnotator->annotate(variableType, logger))
    {
        logger->logSourceError(LogMessage("Invalid variable type {}", variableType->getContent()),
                               variableType->getSourceRef());
        return false;
    }

    size_t variableTypeId = std::dynamic_pointer_cast<TypeAbleAnnotation>(variableType->getAnnotation())->getTypeId();
    std::dynamic_pointer_cast<SymbolAnnotation>(variableName->getAnnotation())->setTypeId(variableTypeId);

    node->setAnnotation(variableName->getAnnotation());
    node->removeChild(); // Name
    node->removeChild(); // Type

    for (size_t i = 0; i < node->getChildren().size(); i++)
    {
        const std::shared_ptr<AstNode> &initializer = node->getChild(i);
        ccAnnotator->setTypeId(variableTypeId);
        if (!ccAnnotator->annotate(initializer, logger))
        {
            logger->logSourceError(LogMessage("Invalid variable initializer"), initializer->getSourceRef());
            return false;
        }
    }

    return true;
}

bool VariableAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Variable().getId();
}
