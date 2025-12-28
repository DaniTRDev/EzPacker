#include "ParsersTestFixture.h"

bool ParsersTestFixture::expectNodeType(AstNodeType type) { return m_parseResult && m_parseResult->getType() == type; }

void ParsersTestFixture::SetUp()
{
    m_parseResult.reset();
    m_logger = EzLogger::createSinkLogger("TEST_PARSERS");
    m_sourceManager = std::make_shared<SourceManager>();
    m_sourceSinkLogger = std::make_shared<SourceLoggingSink>(m_logger.get(), m_sourceManager);
    m_tokenizer = std::make_shared<BasicTokenizer>(m_sourceSinkLogger, m_sourceManager, "TEST_PARSERS");
    m_errorCollector = std::make_shared<ErrorCollector>(m_sourceSinkLogger, m_sourceManager);

    Test::SetUp();
}

void ParsersTestFixture::TearDown()
{
    m_parsingContext.reset();
    m_tokenizer.reset();
    m_sourceSinkLogger.reset();
    m_sourceManager.reset();
    m_logger.reset();

    Test::TearDown();
}

void ParsersTestFixture::tokenizeAndCreateContext(const std::string &input)
{
    m_sourceManager->addSourceContent("TEST_PARSERS", input);
    m_tokenizer->tokenizeBuffer((char *)input.data(), 0, input.size());
    m_parsingContext =
            std::make_shared<SingleThreadParsingContext>(m_errorCollector, m_sourceManager, m_tokenizer->getTokens());
}

const std::shared_ptr<AstNode> &ParsersTestFixture::getParseResult() { return m_parseResult; }