#include "NodeAnnotators/SymbolAnnotator.h"

SymbolAnnotator::SymbolAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable) :
    m_workingMode(SymbolAnnotatorWorkingMode::ExpectsExistingSymbol), m_typeId(0)
{
    setSymbolTable(symbolTable);
}

bool SymbolAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit right away.

    if (!node)
    {
        logger->logError(LogMessage("Invalid Symbolic node"));
        return false;
    }

    const ScopeAbleAnnotator::SymbolTableT &symbolTable = getSymbolTable();
    const std::string &identifier = node->getContent();
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    size_t symbolId = 0;

    if (m_workingMode == SymbolAnnotatorWorkingMode::ExpectsExistingSymbol)
    {
        if (!symbolTable->doesElemExistAtAnyUpperScopeByString(ScopedSymbol::getStringFromName(identifier), symbolId))
        {
            logger->logSourceError(LogMessage("Undefined symbol {}", identifier), sourceRef);
            return false;
        }
    }
    else
    {
        if (symbolTable->doesElemExistByString(ScopedSymbol::getStringFromName(identifier), symbolId))
        {
            logger->logSourceError(LogMessage("Symbol redefinition", identifier), sourceRef);
            return false;
        }

        symbolId = symbolTable->addItem(std::make_shared<ScopedSymbol>(m_typeId, identifier));
    }

    node->setContent(""); // Safety measure.
    node->setAnnotation(std::make_shared<SymbolAnnotation>(symbolId, m_typeId));

    return true;
}

bool SymbolAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Identifier().getId();
}

void SymbolAnnotator::setTypeId(size_t typeId) { m_typeId = typeId; }

void SymbolAnnotator::setWorkingMode(SymbolAnnotatorWorkingMode mode) { m_workingMode = mode; }
