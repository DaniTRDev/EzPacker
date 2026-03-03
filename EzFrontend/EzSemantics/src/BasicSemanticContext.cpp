#include "BasicSemanticContext.h"

BasicSemanticContext::BasicSemanticContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                           const std::shared_ptr<SourceManager> &sourceManager) :
    m_currentLoopNestLevel(0), m_currentSymbolId(1), ErrorEmitter(errorCollector, sourceManager)
{
    m_globalScope = std::make_shared<Scope>(nullptr, "global");
    m_currentScope = m_globalScope.get();

    m_scopes.reserve(1024); // Pre-allocate 1024 scopes to avoid reallocations.
}

bool BasicSemanticContext::createSymbol(AstNode *definingNode,
                                        SymbolType symbolType,
                                        Symbol **outSymbol,
                                        Type *symbolDataType,
                                        const std::string_view &symbolName)
{
    if (resolveSymbolInScope(symbolName, nullptr, false))
    {
        // Symbol already created.
        return false;
    }

    Symbol *sym = m_symbolPool.create<Symbol>(definingNode, symbolDataType, symbolType, symbolName);

    if (!getCurrentScope()->define(sym, symbolName))
    {
        // Mustn't happen, but still it's nice to have.
        return false;
    }

    sym->setId(m_currentSymbolId++);
    *outSymbol = sym;

    return true;
}

bool BasicSemanticContext::isCurrentScopeGlobalScope() const { return m_currentScope->getParent() == nullptr; }

bool BasicSemanticContext::isContextInsideLoop() const { return m_currentLoopNestLevel != 0; }

bool BasicSemanticContext::isSymbolLinkedToMir(Symbol *symbol) const
{
    return m_symbolToMirMap.contains(symbol->getId());
}

bool BasicSemanticContext::linkSymbolToMirId(Symbol *sym, size_t mirId)
{
    if (!sym || m_symbolToMirMap.contains(sym->getId()))
    {
        emitError(ErrorSeverity::Fatal, "Could not link symbol to MIR", "linkSymbolToMirId");
        return false;
    }

    m_symbolToMirMap[sym->getId()] = mirId;
    return true;
}

bool BasicSemanticContext::resolveSymbolInScope(const std::string_view &symbolName,
                                                Symbol **outSymbol,
                                                bool searchParent)
{
    return getCurrentScope()->resolve(symbolName, outSymbol, searchParent);
}

MirId BasicSemanticContext::getMirIdOfSymbol(Symbol *sym) const
{
    if (!sym)
    {
        return MIRID_INVALID;
    }

    auto it = m_symbolToMirMap.find(sym->getId());
    if (it == m_symbolToMirMap.end())
    {
        return MIRID_INVALID;
    }

    return it->second;
}

Scope *BasicSemanticContext::getCurrentScope() const { return m_currentScope; }

TypedPool *BasicSemanticContext::getAnnotPool() { return &m_annotationPool; }

TypedPool *BasicSemanticContext::getSymbolPool() { return &m_symbolPool; }

void BasicSemanticContext::beginScope(const std::string_view &name)
{
    std::shared_ptr<Scope> scope = std::make_shared<Scope>(getCurrentScope(), name);
    m_scopes.push_back(scope);

    if (m_scopes.size() == 1024)
    {
        // Reserve another 1024 scopes in advance.
        m_scopes.reserve(1024);
    }

    m_currentScope = scope.get();
}

void BasicSemanticContext::emitSymbolRedefinitionError(const std::string_view &module,
                                                       const std::string_view &symbolName,
                                                       AstNode *errorNode)
{
    Symbol *symbol;
    if (!resolveSymbolInScope(symbolName, &symbol, false))
    {
        throw std::runtime_error("Internal Compiler Error: Symbol should be defined but it's not.");
    }

    const SourceReference &errorSourceRef = errorNode->getSourceRef(),
                          &definingSourceRef = symbol->getDefiningNode()->getSourceRef();

    std::string moduleCopy = std::string(module);
    emitError(ErrorSeverity::Fatal, std::format("Redefinition of symbol '{}'", symbolName), moduleCopy, errorSourceRef);
    emitError(ErrorSeverity::Fatal, "Previously defined here", moduleCopy, definingSourceRef);
}

void BasicSemanticContext::emitUnknownSymbolError(const std::string_view &module,
                                                  const std::string_view &symbolName,
                                                  AstNode *errorNode)
{
    std::string moduleCopy = std::string(module);
    emitError(ErrorSeverity::Fatal,
              std::format("Unknown symbol '{}'", symbolName),
              moduleCopy,
              errorNode->getSourceRef());
}

void BasicSemanticContext::endScope()
{
    if (isCurrentScopeGlobalScope())
    {
        throw std::runtime_error("Internal Compiler Error: Can't end the global scope");
    }

    exitScope();
}

void BasicSemanticContext::enterLoop() { m_currentLoopNestLevel++; }

void BasicSemanticContext::enterScope(Scope *scope) { m_currentScope = scope; }

void BasicSemanticContext::exitLoop()
{
    if (m_currentLoopNestLevel == 0)
    {
        throw std::runtime_error("Internal Compiler Error: Can't exit loop when not in a loop");
    }

    m_currentLoopNestLevel--;
}

void BasicSemanticContext::exitScope()
{
    if (isCurrentScopeGlobalScope())
    {
        throw std::runtime_error("Internal Compiler Error: Can't exist global scope");
    }

    m_currentScope = m_currentScope->getParent();
}

const std::shared_ptr<Scope> &BasicSemanticContext::getGlobalScope() const { return m_globalScope; }
