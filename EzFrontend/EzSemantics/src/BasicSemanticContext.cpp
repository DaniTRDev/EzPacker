#include "BasicSemanticContext.h"

BasicSemanticContext::BasicSemanticContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                           const std::shared_ptr<SourceManager> &sourceManager) :
    m_currentSymbolId(1), ErrorEmitter(errorCollector, sourceManager)
{
    m_globalScope = std::make_shared<Scope>(nullptr, "global");
    m_currentScope = m_globalScope;
}

bool BasicSemanticContext::createSymbol(SymbolType symbolType,
                                        const std::shared_ptr<AstNode> &definingNode,
                                        std::shared_ptr<Symbol> *outSymbol,
                                        const std::shared_ptr<Type> &symbolDataType,
                                        const std::string &symbolName)
{
    if (resolveSymbolInScope(symbolName, nullptr, false))
    {
        // Symbol already created.
        return false;
    }

    auto symbol = std::make_shared<Symbol>(symbolType, definingNode, symbolDataType, symbolName);
    if (!getCurrentScope()->define(symbolName, symbol))
    {
        // Mustn't happen, but still it's nice to have.
        return false;
    }

    symbol->setId(m_currentSymbolId++);
    *outSymbol = symbol;

    return true;
}

bool BasicSemanticContext::isCurrentScopeGlobalScope() const { return m_currentScope->getParent() == nullptr; }

bool BasicSemanticContext::resolveSymbolInScope(const std::string &symbolName,
                                                std::shared_ptr<Symbol> *outSymbol,
                                                bool searchParent)
{
    return getCurrentScope()->resolve(symbolName, outSymbol, searchParent);
}

void BasicSemanticContext::beginScope(const std::string &name)
{
    std::shared_ptr<Scope> scope = std::make_shared<Scope>(getCurrentScope(), name);
    m_currentScope = scope;
}

void BasicSemanticContext::emitSymbolRedefinitionError(const std::string &module,
                                                       const std::string &symbolName,
                                                       const std::shared_ptr<AstNode> &errorNode)
{
    std::shared_ptr<Symbol> symbol;
    if (!resolveSymbolInScope(symbolName, &symbol, false))
    {
        throw std::runtime_error(
                "Internal compiler error in emitSymbolRedefinitionError. Symbol should be defined but it's not.");
    }

    std::shared_ptr<SourceReference> errorSourceRef = errorNode->getFirstSourceReference(),
                                     definingSourceRef = symbol->getDefiningNode()->getFirstSourceReference();

    emitError(ErrorSeverity::Fatal, std::format("Redefinition of symbol '{}'", symbolName), module, errorSourceRef);
    emitError(ErrorSeverity::Fatal, "Previously defined here", module, definingSourceRef);
}

void BasicSemanticContext::emitUnknownSymbolError(const std::string &module,
                                                  const std::string &symbolName,
                                                  const std::shared_ptr<AstNode> &errorNode)
{
    emitError(ErrorSeverity::Fatal,
              std::format("Unknown symbol '{}'", symbolName),
              module,
              errorNode->getFirstSourceReference());
}

void BasicSemanticContext::endScope()
{
    if (isCurrentScopeGlobalScope())
    {
        throw std::runtime_error("Can't end the global scope");
    }

    exitScope();
}

void BasicSemanticContext::enterScope(const std::shared_ptr<Scope> &scope) { m_currentScope = scope; }

void BasicSemanticContext::exitScope()
{
    if (isCurrentScopeGlobalScope())
    {
        throw std::runtime_error("Can't exist global scope");
    }

    m_currentScope = m_currentScope->getParent();
}

const std::shared_ptr<Scope> &BasicSemanticContext::getCurrentScope() const { return m_currentScope; }

const std::shared_ptr<Scope> &BasicSemanticContext::getGlobalScope() const { return m_globalScope; }
