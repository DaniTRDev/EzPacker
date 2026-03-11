#include "Frontend/EzFrontendWrapper.h"
#include "CompilationPhases/IncludePhase.h"

EzFrontendWrapper::EzFrontendWrapper(std::shared_ptr<SourceLoggingSink> sink) : m_sink(std::move(sink))
{
    rebuildFrontendState(std::filesystem::current_path());
}

void EzFrontendWrapper::onDiagnostic(void *userParam, const std::shared_ptr<Error> &error)
{
    auto *self = static_cast<EzFrontendWrapper *>(userParam);
    if (!self || !error)
    {
        return;
    }

    self->m_diagnostics.push_back(FrontendDiagnosticEntry{ .m_severity = error->m_severity,
                                                           .m_sourceRef = error->m_sourceRef,
                                                           .m_message = error->m_message,
                                                           .m_sender = error->m_sender,
                                                           .m_timeStamp = error->m_timeStamp });
}

void EzFrontendWrapper::rebuildFrontendState(const std::filesystem::path &workingDirectory)
{
    m_workingDirectory = workingDirectory.empty() ? std::filesystem::current_path() : workingDirectory;
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_sourceManager = std::make_shared<SourceManager>(m_workingDirectory);
    m_compilerDriver = std::make_shared<FrontendCompilerDriver>(m_errorCollector, m_sourceManager);
    m_errorCollector->addSubscriber(&EzFrontendWrapper::onDiagnostic, this);
    clearCompilationArtifacts();
}

void EzFrontendWrapper::clearCompilationArtifacts()
{
    m_compilationUnit.reset();
    m_lastCompilationSucceeded = false;
    m_cachedParseResult.clear();
    m_diagnostics.clear();
    m_includedFiles.clear();
    rebuildPipelineStages();
}

bool EzFrontendWrapper::loadSourceFromFile(const std::filesystem::path &filePath)
{
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath))
    {
        return false;
    }

    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open())
    {
        return false;
    }

    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    m_sourceText.assign(static_cast<size_t>(fileSize), '\0');
    if (fileSize > 0 && !fileStream.read(m_sourceText.data(), fileSize))
    {
        return false;
    }

    m_loadedFilePath = std::filesystem::absolute(filePath);
    m_sourceName = m_loadedFilePath.filename().string();
    rebuildFrontendState(m_loadedFilePath.parent_path());
    return true;
}

bool EzFrontendWrapper::saveSourceToFile(const std::filesystem::path &filePath, std::string_view sourceText) const
{
    std::ofstream fileStream(filePath, std::ios::binary | std::ios::trunc);
    if (!fileStream.is_open())
    {
        return false;
    }

    fileStream.write(sourceText.data(), static_cast<std::streamsize>(sourceText.size()));
    return fileStream.good();
}

bool EzFrontendWrapper::compileSource(const std::string &sourceText,
                                     const std::string &sourceName,
                                     const std::filesystem::path &workingDirectory)
{
    rebuildFrontendState(workingDirectory);
    m_sourceText = sourceText;
    m_sourceName = sourceName.empty() ? "<untitled>.ez" : sourceName;

    m_errorCollector->beginScope();
    bool addOk = m_compilerDriver->addSource(m_sourceText, m_sourceName, &m_compilationUnit);
    bool compileOk = addOk && m_compilerDriver->compile();
    bool hasFatalErrors = m_errorCollector->doesCurrentScopeHasFatalErrors();
    m_errorCollector->endScope(ErrorAction::Commit);

    m_lastCompilationSucceeded = addOk && compileOk && !hasFatalErrors && m_compilationUnit != nullptr;

    if (m_lastCompilationSucceeded)
    {
        collectIncludeFiles();
    }

    rebuildPipelineStages();
    return m_lastCompilationSucceeded;
}

void EzFrontendWrapper::collectIncludeFiles()
{
    m_includedFiles.clear();
    if (!m_compilationUnit || !m_compilationUnit->getGlobalScopeAstNodes())
    {
        return;
    }

    IncludePhase includePhase;
    m_errorCollector->beginScope();
    if (includePhase.execute(m_compilationUnit.get()))
    {
        std::set<std::string_view> includedFiles;
        includePhase.moveIncludedFilesToDest(includedFiles);
        for (const std::string_view file : includedFiles)
        {
            m_includedFiles.emplace_back(file);
        }
    }
    m_errorCollector->endScope(ErrorAction::Discard);
}

void EzFrontendWrapper::rebuildPipelineStages()
{
    m_pipelineStages.clear();

    const bool hasUnit = m_compilationUnit != nullptr;
    const bool hasTokenizer = hasUnit && m_compilationUnit->getTokenizer() != nullptr;
    const bool hasAst = hasUnit && m_compilationUnit->getGlobalScopeAstNodes() != nullptr;
    const bool hasSemantics = hasUnit && m_compilationUnit->getSemanticContext() != nullptr;
    const bool hasMir = hasUnit && m_compilationUnit->getMirEmitterContext() != nullptr && m_compilationUnit->getLoweringContext() != nullptr;

    m_pipelineStages.push_back({ "Source", !m_sourceText.empty(), std::format("{} bytes", m_sourceText.size()) });
    m_pipelineStages.push_back({ "Tokenization", hasTokenizer, hasTokenizer ? std::format("{} tokens", getTokens().size()) : "No token stream" });
    m_pipelineStages.push_back({ "Parsing", hasAst, hasAst ? std::format("{} top-level nodes", getParseResult().size()) : "No AST" });
    m_pipelineStages.push_back({ "Include Resolution", hasUnit, m_includedFiles.empty() ? "No includes" : std::format("{} includes", m_includedFiles.size()) });
    m_pipelineStages.push_back({ "Semantic Analysis", hasSemantics, hasSemantics ? "Context and scopes available" : "No semantic context" });
    m_pipelineStages.push_back({ "AST Lowering / MIR", hasMir, hasMir ? "MIR context available" : "No MIR emitted" });
    m_pipelineStages.push_back({ "Compilation", m_lastCompilationSucceeded, m_lastCompilationSucceeded ? "Success" : "Failed or not run" });
}

const std::shared_ptr<ErrorCollector> &EzFrontendWrapper::getErrorCollector() const { return m_errorCollector; }

const std::shared_ptr<SourceLoggingSink> &EzFrontendWrapper::getLoggingSink() const { return m_sink; }

const std::shared_ptr<SourceManager> &EzFrontendWrapper::getSourceManager() const { return m_sourceManager; }

const std::shared_ptr<FrontendCompilationUnit> &EzFrontendWrapper::getCompilationUnit() const { return m_compilationUnit; }

const std::vector<TokenInformation> &EzFrontendWrapper::getTokens() const
{
    static const std::vector<TokenInformation> empty;
    return (m_compilationUnit && m_compilationUnit->getTokenizer()) ? m_compilationUnit->getTokenizer()->getTokens() : empty;
}

const std::vector<AstNode *> &EzFrontendWrapper::getParseResult() const
{
    m_cachedParseResult.clear();
    if (m_compilationUnit && m_compilationUnit->getGlobalScopeAstNodes())
    {
        for (AstNode *node : *m_compilationUnit->getGlobalScopeAstNodes())
        {
            m_cachedParseResult.push_back(node);
        }
    }
    return m_cachedParseResult;
}

const std::vector<FrontendDiagnosticEntry> &EzFrontendWrapper::getDiagnostics() const { return m_diagnostics; }

const std::vector<std::string> &EzFrontendWrapper::getIncludedFiles() const { return m_includedFiles; }

const std::vector<FrontendPipelineStage> &EzFrontendWrapper::getPipelineStages() const { return m_pipelineStages; }

const std::filesystem::path &EzFrontendWrapper::getLoadedFilePath() const { return m_loadedFilePath; }

const std::string &EzFrontendWrapper::getLoadedSourceText() const { return m_sourceText; }

const std::string &EzFrontendWrapper::getSourceName() const { return m_sourceName; }

const std::filesystem::path &EzFrontendWrapper::getWorkingDirectory() const { return m_workingDirectory; }

bool EzFrontendWrapper::hasCompilationResult() const { return m_compilationUnit != nullptr; }

bool EzFrontendWrapper::lastCompilationSucceeded() const { return m_lastCompilationSucceeded; }
