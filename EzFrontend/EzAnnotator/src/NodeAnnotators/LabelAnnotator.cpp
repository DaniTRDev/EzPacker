#include "NodeAnnotators/LabelAnnotator.h"

LabelAnnotator::LabelAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                               const ScopeAbleAnnotator::TypeTableT &typeTable)
{
    setSymbolTable(symbolTable);
    setTypeTable(typeTable);
}

bool LabelAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid label node"));
        return false;
    }

    const std::shared_ptr<AstNode> &identifierNode = node->getChild(0);
    auto iiAnnotator = std::make_shared<InstructionAnnotator>(getSymbolTable(), getTypeTable());
    std::shared_ptr<SymbolAnnotator> ssAnnotator = std::make_shared<SymbolAnnotator>(getSymbolTable());

    ssAnnotator->setWorkingMode(SymbolAnnotatorWorkingMode::CreateNewSymbol);
    if (!ssAnnotator->annotate(identifierNode, logger))
    {
        logger->logError(LogMessage("Invalid label name {}", identifierNode->getContent()));
        return false;
    }

    size_t labelId = std::dynamic_pointer_cast<SymbolAnnotation>(identifierNode->getAnnotation())->getSymbolId();

    node->setAnnotation(std::make_shared<LabelAnnotation>(labelId)); // Move the symbol annot to the label node.
    node->removeChild();                                             // Remove label's symbol identifier.

    // Now start with label's instruction.
    for (size_t i = 0; i < node->getChildren().size(); i++)
    {
        const std::shared_ptr<AstNode> &labelInstr = node->getChild(i);
        if (!iiAnnotator->annotate(labelInstr, logger))
        {
            logger->logSourceError(LogMessage("Invalid instruction"), labelInstr->getSourceRef());
            return false;
        }
    }

    return true;
}

bool LabelAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Label().getId();
}
