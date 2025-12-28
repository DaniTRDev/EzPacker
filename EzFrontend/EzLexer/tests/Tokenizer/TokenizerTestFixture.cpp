#include "TokenizerTestFixture.h"

bool TokenizerTestFixture::advanceToken()
{
    if (!m_tokenizer || m_currentPos >= m_tokenizer->getTokens().size())
        return false;

    return (++m_currentPos < m_tokenizer->getTokens().size());
}

bool TokenizerTestFixture::expectTokenContent(std::string content)
{
    if (!m_tokenizer || m_currentPos >= m_tokenizer->getTokens().size())
        return false;

    return m_tokenizer->getTokens()[m_currentPos].m_str == content;
}

bool TokenizerTestFixture::expectTokenCount(size_t count)
{
    if (!m_tokenizer)
        return false;

    return m_tokenizer->getTokens().size() == count;
}

bool TokenizerTestFixture::expectTokenizeResult(const std::string &content)
{
    m_sourceManager->addSourceContent("TEST_SOURCE", content);
    return m_tokenizer->tokenizeBuffer((char *)content.data(), 0, content.size());
}

bool TokenizerTestFixture::expectTokenType(_TokenType type)
{
    if (!m_tokenizer || m_currentPos >= m_tokenizer->getTokens().size())
        return false;

    return m_tokenizer->getTokens()[m_currentPos].m_type == type;
}

void TokenizerTestFixture::SetUp()
{
    m_currentPos = 0;
    m_logger = EzLogger::createSinkLogger("TEST");
    m_sourceManager = std::make_shared<SourceManager>();
    m_loggingSink = std::make_shared<SourceLoggingSink>(m_logger.get());

    Test::SetUp();
}

void TokenizerTestFixture::TearDown()
{
    m_logger.reset();
    m_sourceManager.reset();
    m_loggingSink.reset();

    Test::TearDown();
}

std::shared_ptr<ITokenizer> TokenizerTestFixture::createBasicTokenizer()
{
    m_tokenizer = std::make_shared<BasicTokenizer>(m_loggingSink, m_sourceManager, "TEST_SOURCE");
    return m_tokenizer;
}
