#include "FrontendCompilerTestFixture.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SetUp / TearDown
// ─────────────────────────────────────────────────────────────────────────────

void FrontendCompilerTestFixture::SetUp()
{
    m_logger = EzLogger::createSyncLogger("FRONTEND_COMPILER_TEST");
    m_sourceManager = std::make_shared<SourceManager>();
    m_sourceSinkLogger = std::make_shared<SourceLoggingSink>(m_logger.get());
    m_errorCollector = std::make_shared<ErrorCollector>();

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
    m_errorCollector->endScope(ErrorAction::Propagate);
    m_errorCollector.reset();
    m_sourceSinkLogger.reset();
    m_sourceManager.reset();
    m_logger.reset();

    Test::TearDown();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Compilation-unit helpers
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<FrontendCompilationUnit> FrontendCompilerTestFixture::createUnit(const std::string &sourceContent,
                                                                                 const std::string &sourceName)
{
    auto unit = std::make_unique<FrontendCompilationUnit>(m_errorCollector, m_sourceManager);
    if (!unit->create(sourceContent, sourceName))
    {
        ADD_FAILURE() << "Failed to create compilation unit for source: " << sourceName;
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

bool FrontendCompilerTestFixture::runThroughParsing(FrontendCompilationUnit *unit)
{
    if (!runTokenization(unit))
        return false;
    ParsingPhase phase;
    return phase.execute(unit);
}

bool FrontendCompilerTestFixture::runThroughSemantics(FrontendCompilationUnit *unit)
{
    if (!runThroughParsing(unit))
        return false;
    SemanticAnalysisPhase phase(nullptr); // No global scope is needed for these tests.
    return phase.execute(unit);
}

bool FrontendCompilerTestFixture::runFullPipeline(FrontendCompilationUnit *unit)
{
    if (!runThroughSemantics(unit))
        return false;
    AstLoweringPhase phase;
    return phase.execute(unit);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Driver helpers
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<FrontendCompilerDriver> FrontendCompilerTestFixture::createDriver()
{
    return std::make_unique<FrontendCompilerDriver>(m_errorCollector, m_sourceManager);
}

// ─────────────────────────────────────────────────────────────────────────────
//  File helpers
// ─────────────────────────────────────────────────────────────────────────────

std::filesystem::path FrontendCompilerTestFixture::getProgramsDir()
{
    std::filesystem::path thisFile(__FILE__);
    return thisFile.parent_path() / "programs";
}

std::string FrontendCompilerTestFixture::readProgramFile(const std::string &fileName)
{
    auto path = getProgramsDir() / fileName;
    std::ifstream ifs(path, std::ios::in);
    if (!ifs.is_open())
    {
        ADD_FAILURE() << "Could not open test program: " << path.string();
        return "";
    }
    return { std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Error state helpers
// ─────────────────────────────────────────────────────────────────────────────

bool FrontendCompilerTestFixture::hasFatalErrors() const { return m_errorCollector->doesCurrentScopeHasFatalErrors(); }
