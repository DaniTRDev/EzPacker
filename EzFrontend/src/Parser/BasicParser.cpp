#include "parser/BasicParser.h"

BasicParser::BasicParser(const std::vector<TokenInformation> &tokens)
    : m_position(0), m_tokens(tokens),
      LogSink(g_logger.get(), LogSegment("PARSER").colorize(Colors::underline))
{
}

BasicParser::~BasicParser()
{
    while (!m_logStack.empty())
        endLogBlock(false);

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

    logParserError(
        LogMessage("").add("Expected type '{}' but got '{}'", g_IRTokenTypeStr[token], g_IRTokenTypeStr[peek().m_type]),
        peek());
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

    static TokenInformation invalid = TokenInformation{.m_type = IRTokenType::Invalid, .m_str = ""};
    return invalid;
}

size_t BasicParser::getPosition() const
{
    return m_position;
}

void BasicParser::beginLogBlock()
{
    m_logStack.push(std::queue<LogMessage>{});
}

void BasicParser::endLogBlock(bool commit)
{
    if (m_logStack.empty())
        return;

    auto logs = std::move(m_logStack.top());
    m_logStack.pop();

    while (!logs.empty())
    {
        if (commit)
            LogSink::pushLog(logs.front());

        logs.pop();
    }
}

void BasicParser::logParserError(const LogMessage &logMessage, const TokenInformation &token)
{
    LogMessage msg = LogMessage("")
                         .add("Parsing error at line {}, column {} -> ", token.m_line, token.m_col)
                         .colorize(Colors::red)
                         .add(logMessage.getRawMessage());
    if (m_logStack.empty())
        LogSink::pushLog(msg);
    else
        m_logStack.top().push(msg);
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