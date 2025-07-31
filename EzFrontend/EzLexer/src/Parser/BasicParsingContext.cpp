#include "Parser/BasicParsingContext.h"

BasicParsingContext::BasicParsingContext(const std::shared_ptr<SourceLoggingSink> &logger,
                                         const std::shared_ptr<SourceManager> &sourceManager) :
    m_position(0), m_logger(logger), m_errorCollector(std::make_shared<ErrorCollector>(logger, sourceManager))
{
}

bool BasicParsingContext::consume()
{
    if (m_position < m_tokens.size())
    {
        m_position++;
        return true;
    }

    return false;
}

bool BasicParsingContext::consumeIfToken(_TokenType token)
{
    skipUselessTokens();

    if (peek().m_type == token)
    {
        consume();
        return true;
    }

    // FIXME: TODO: Remove the IRTokenTypeStr thing.
    m_errorCollector->error(
            LogMessage("Expected token '{}' but got '{}'", TokenType2StrMap[token], TokenType2StrMap[peek().m_type]),
            peek().m_sourceReference);
    return false;
}

bool BasicParsingContext::restore(size_t pos)
{
    if (pos >= m_tokens.size())
        return false;

    m_position = pos;
    return true;
}

const TokenInformation &BasicParsingContext::peek() const
{
    if (m_position < m_tokens.size())
        return m_tokens[m_position];

    static auto invalid = TokenInformation{ .m_type = _TokenType::Invalid, .m_sourceReference = nullptr, .m_str = "" };
    return invalid;
}

size_t BasicParsingContext::getPosition() const { return m_position; }

void BasicParsingContext::setTokens(const std::vector<TokenInformation> &tokens) { m_tokens = tokens; }

void BasicParsingContext::skipUselessTokens()
{
    auto token = peek(); // Might not be performant due to copies, ... With const we can't make it a reference.
    while (token.m_type == _TokenType::WhiteSpace || token.m_type == _TokenType::NewLine ||
           token.m_type == _TokenType::Comment)
    {
        consume();
        token = peek();
    }
}

const std::shared_ptr<ErrorCollector> &BasicParsingContext::getErrorCollector() const { return m_errorCollector; }
