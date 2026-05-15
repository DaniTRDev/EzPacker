#include "FrontendCompilationUnit.h"

FrontendCompilationUnit::FrontendCompilationUnit(const std::shared_ptr<ErrorCollector> &errorCollector,
                                                 const std::shared_ptr<SourceManager> &sourceManager) :
    ErrorEmitter(errorCollector, sourceManager)
{
    cleanup();
}

bool FrontendCompilationUnit::create(size_t sourceId)
{
    cleanup();
    m_targetSourceId = sourceId;
    m_globalScope = std::make_shared<Scope>(
            nullptr,
            std::format("@global_scope@{}@{}", getSourceManager()->getSourceName(sourceId), sourceId));
    return true;
}

size_t FrontendCompilationUnit::getTargetSourceId() const { return m_targetSourceId; }

const std::shared_ptr<Scope> &FrontendCompilationUnit::getGlobalScope() const { return m_globalScope; }

TypedPoolLinkedList<AstNode> *FrontendCompilationUnit::getGlobalScopeAstNodes() { return m_globalScopeAstNodes; }

void FrontendCompilationUnit::cleanup()
{
    m_globalScopeAstNodes = nullptr;
    m_targetSourceId = 0;
    m_parsingContext.reset();
    m_semanticContext.reset();
    m_tokenizer.reset();
    m_loweringContext.reset();
    m_mirEmitter.reset();
    m_mirEmitterContext.reset();
}

void FrontendCompilationUnit::setGlobalScopeAstNodes(TypedPoolLinkedList<AstNode> *globalScopeAstNodes)
{
    m_globalScopeAstNodes = globalScopeAstNodes;
}

void FrontendCompilationUnit::setLoweringContext(const std::shared_ptr<AstLoweringContext> &loweringContext)
{
    m_loweringContext = loweringContext;
}

void FrontendCompilationUnit::setMirEmitter(const std::shared_ptr<MirEmitter> &mirEmitter)
{
    m_mirEmitter = mirEmitter;
}

void FrontendCompilationUnit::setMirEmitterContext(const std::shared_ptr<MirEmitterContext> &mirEmitterContext)
{
    m_mirEmitterContext = mirEmitterContext;
}


void FrontendCompilationUnit::setParsingContext(const std::shared_ptr<BasicParsingContext> &parsingContext)
{
    m_parsingContext = parsingContext;
}

void FrontendCompilationUnit::setSemanticContext(const std::shared_ptr<BasicSemanticContext> &semanticContext)
{
    m_semanticContext = semanticContext;
}

void FrontendCompilationUnit::setTokenizer(const std::shared_ptr<BasicTokenizer> &tokenizer)
{
    m_tokenizer = tokenizer;
}

const std::shared_ptr<BasicParsingContext> &FrontendCompilationUnit::getParsingContext() const
{
    return m_parsingContext;
}

const std::shared_ptr<BasicSemanticContext> &FrontendCompilationUnit::getSemanticContext() const
{
    return m_semanticContext;
}

const std::shared_ptr<BasicTokenizer> &FrontendCompilationUnit::getTokenizer() const { return m_tokenizer; }

const std::shared_ptr<AstLoweringContext> &FrontendCompilationUnit::getLoweringContext() const
{
    return m_loweringContext;
}

const std::shared_ptr<MirEmitter> &FrontendCompilationUnit::getMirEmitter() const { return m_mirEmitter; }

const std::shared_ptr<MirEmitterContext> &FrontendCompilationUnit::getMirEmitterContext() const
{
    return m_mirEmitterContext;
}