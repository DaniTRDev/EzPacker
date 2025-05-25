#include "parser/BasicParser.h"

BasicParser::BasicParser(const std::shared_ptr<FrontendLogger> &logger,
                         const std::shared_ptr<SourceManager> &sourceManager,
                         const std::vector<TokenInformation> &tokens)
    : m_position(0), m_logger(logger), m_tokens(tokens),
      m_errorCollector(std::make_shared<ErrorCollector>(logger, sourceManager))
{
}

BasicParser::~BasicParser()
{
    m_tokens.clear();
}

bool BasicParser::consume()
{
    if (m_position < m_tokens.size())
    {
        m_position++;
        return true;
    }

    return false;
}

bool BasicParser::consumeIfToken(IRTokenType token)
{
    if (peek().m_type == token)
    {
        consume();
        return true;
    }

    m_errorCollector->collect(LogMessage("").add("Expected token '{}' but got '{}'", g_IRTokenTypeStr[token],
                                                 g_IRTokenTypeStr[peek().m_type]),
                              peek().m_sourceReference);
    return false;
}

bool BasicParser::restore(size_t pos)
{
    if (pos >= m_tokens.size())
        return false;

    m_position = pos;
    return true;
}

const TokenInformation &BasicParser::peek() const
{
    if (m_position < m_tokens.size())
        return m_tokens[m_position];

    static TokenInformation invalid = TokenInformation{
        .m_type = IRTokenType::Invalid, .m_sourceReference = std::make_shared<SourceReference>(), .m_str = ""};
    return invalid;
}

size_t BasicParser::getPosition() const
{
    return m_position;
}

void BasicParser::skipWhiteSpacesAndNewLines()
{
    auto token = peek(); // Might not be performant due to copies, ... With const we can't make it a reference.
    while (token.m_type == IRTokenType::WhiteSpace || token.m_type == IRTokenType::NewLine ||
           token.m_type == IRTokenType::Comment)
    {
        consume();
        token = peek();
    }
}

const std::shared_ptr<ErrorCollector> &BasicParser::getErrorCollector()
{
    return m_errorCollector;
}