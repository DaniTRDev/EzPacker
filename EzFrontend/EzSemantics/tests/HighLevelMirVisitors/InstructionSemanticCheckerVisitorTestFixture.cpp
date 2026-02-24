#include "InstructionSemanticCheckerVisitorTestFixture.h"

void InstructionSemanticCheckerVisitorTestFixture::SetUp()
{
    m_logger = EzLogger::createSyncLogger("TEST_SEMANTICS");
    m_sourceManager = std::make_shared<SourceManager>();
    m_sourceSinkLogger = std::make_shared<SourceLoggingSink>(m_logger.get());
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_semanticContext = std::make_shared<BasicSemanticContext>(m_errorCollector, m_sourceManager);

    m_errorCollector->addSubscriber(
            [](void *userParam, const std::shared_ptr<Error> &error) -> void
            {
                InstructionSemanticCheckerVisitorTestFixture *fixture = (InstructionSemanticCheckerVisitorTestFixture *)userParam;
                if (error->m_sourceRef)
                {
                    g_logger->pushLog(LogMessage("[{}]{} {}:{}:{} {} \n\t {}",
                                                 error->m_sender,
                                                 error->m_timeStamp,
                                                 error->m_sourceRef->m_sourceFile,
                                                 error->m_sourceRef->m_line,
                                                 error->m_sourceRef->m_col,
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

void InstructionSemanticCheckerVisitorTestFixture::TearDown()
{
    m_visitor.reset();
    m_semanticContext.reset();
    m_sourceSinkLogger.reset();
    m_sourceManager.reset();
    m_logger.reset();

    Test::TearDown();
}
