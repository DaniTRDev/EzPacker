#include "FrontendCompilerTestFixture.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SetUp / TearDown
// ─────────────────────────────────────────────────────────────────────────────

void FrontendCompilerTestFixture::SetUp()
{
    m_logger = EzLogger::createSyncLogger("FC_TEST");
    m_sourceManager = std::make_shared<SourceManager>(getProgramsDir());
    m_sourceSinkLogger = std::make_shared<SourceLoggingSink>(m_logger.get());
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_driver = std::make_shared<FrontendCompilerDriver>(m_errorCollector, m_sourceManager);

    m_errorCollector->beginScope();
    m_errorCollector->addSubscriber(
            [](void *userParam, const std::shared_ptr<Error> &error) -> void
            {
                auto *fixture = static_cast<FrontendCompilerTestFixture *>(userParam);
                if (error->m_sourceRef.m_valid)
                {
                    g_logger->pushLog(
                            LogMessage("[{}]{} {}:{}:{} {} \n\t {}",
                                       error->m_sender,
                                       error->m_timeStamp,
                                       fixture->m_sourceManager->getSourceName(error->m_sourceRef.m_sourceFileId),
                                       error->m_sourceRef.m_line,
                                       error->m_sourceRef.m_col,
                                       error->m_message,
                                       fixture->m_sourceManager->getReferenceContent(error->m_sourceRef)));
                }
                else
                {
                    g_logger->pushLog(LogMessage("[{}]{} {}", error->m_sender, error->m_timeStamp, error->m_message));
                }
            },
            this);

    Test::SetUp();
}

void FrontendCompilerTestFixture::TearDown()
{
    m_errorCollector->endScope(ErrorAction::Commit);
    m_driver.reset();
    m_errorCollector.reset();
    m_sourceSinkLogger.reset();
    m_sourceManager.reset();
    m_logger.reset();

    Test::TearDown();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Compilation unit helpers
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<FrontendCompilationUnit> FrontendCompilerTestFixture::createUnitFromSource(const std::string &content,
                                                                                           const std::string &name)
{
    size_t id = m_sourceManager->addSourceContent(name, content);
    if (id == 0)
    {
        ADD_FAILURE() << "Failed to register source content: " << name;
        return nullptr;
    }

    auto unit = std::make_shared<FrontendCompilationUnit>(m_errorCollector, m_sourceManager);
    if (!unit->create(id))
    {
        ADD_FAILURE() << "FrontendCompilationUnit::create failed for source: " << name;
        return nullptr;
    }

    return unit;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Phase runners
// ─────────────────────────────────────────────────────────────────────────────

bool FrontendCompilerTestFixture::runTokenization(FrontendCompilationUnit *unit)
{
    TokenizationPhase phase;
    return phase.execute(unit);
}

bool FrontendCompilerTestFixture::runUpToParsing(FrontendCompilationUnit *unit)
{
    if (!runTokenization(unit))
        return false;

    ParsingPhase phase;
    return phase.execute(unit);
}

bool FrontendCompilerTestFixture::runUpToSemantics(FrontendCompilationUnit *unit)
{
    if (!runUpToParsing(unit))
        return false;

    // Set up the semantic context before running semantic analysis
    auto semanticContext =
            std::make_shared<BasicSemanticContext>(m_errorCollector, m_sourceManager, unit->getGlobalScope());
    unit->setSemanticContext(semanticContext);

    SemanticAnalysisPhase phase;
    return phase.execute(unit);
}

bool FrontendCompilerTestFixture::runFullSingleUnit(FrontendCompilationUnit *unit)
{
    if (!runUpToSemantics(unit))
        return false;

    AstLoweringPhase phase;
    return phase.execute(unit);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Compiler driver helpers
// ─────────────────────────────────────────────────────────────────────────────

bool FrontendCompilerTestFixture::compileFromSource(const std::string &content, const std::string &name)
{
    if (!m_driver->addSource(content, name))
        return false;
    return m_driver->compile();
}

bool FrontendCompilerTestFixture::compileFromFile(const std::string &fileName)
{
    if (!m_driver->addSourceFromFile(fileName))
        return false;
    return m_driver->compile();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<ErrorCollector> FrontendCompilerTestFixture::getErrorCollector() const { return m_errorCollector; }
std::shared_ptr<SourceManager> FrontendCompilerTestFixture::getSourceManager() const { return m_sourceManager; }
FrontendCompilerDriver *FrontendCompilerTestFixture::getDriver() const { return m_driver.get(); }

bool FrontendCompilerTestFixture::hasFatalErrors() const { return m_errorCollector->doesCurrentScopeHasFatalErrors(); }

std::filesystem::path FrontendCompilerTestFixture::getProgramsDir()
{
    // __FILE__ resolves to .../tests/FrontendCompilerTestFixture.cpp
    std::filesystem::path thisFile(__FILE__);
    return thisFile.parent_path() / "programs";
}
