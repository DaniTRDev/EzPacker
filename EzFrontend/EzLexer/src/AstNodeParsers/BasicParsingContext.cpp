#include "AstNodeParsers/BasicParsingContext.h"

BasicParsingContext::BasicParsingContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                         const std::shared_ptr<SourceManager> &sourceManager,
                                         std::vector<TokenInformation> tokens) :
    m_currentPos(0), m_tokens(std::move(tokens)), ErrorEmitter(errorCollector, sourceManager)
{
}

AstNodeTypedPool *BasicParsingContext::getNodePool() { return &m_nodePool; }

bool BasicParsingContext::canPeek() const { return m_currentPos < m_tokens.size(); }

StringPool *BasicParsingContext::getStringPool() { return &m_stringPool; }

size_t BasicParsingContext::getCurrentPosition() const { return m_currentPos; }

size_t BasicParsingContext::getRemainingTokenCount() const
{
    if (m_tokens.size() > m_currentPos)
        return m_tokens.size() - m_currentPos;
    else
        return 0;
}

const SourceReference &BasicParsingContext::getLastSourceReference() const { return m_lastSourceRef; }

const TokenInformation &BasicParsingContext::peek() const { return m_tokens.data()[getCurrentPosition()]; }

void BasicParsingContext::consume()
{
    if (canPeek())
    {
        m_lastSourceRef = m_tokens[m_currentPos].m_sourceReference;
        m_currentPos++;

        if (canPeek())
        {
            bool shouldSkip = (peek().m_type == _TokenType::NewLine) || (peek().m_type == _TokenType::Tab);

            // If there's a tab or a newline, we skip it.
            if (shouldSkip)
                consume();
        }
    }
}

void BasicParsingContext::setPosition(size_t pos)
{
    if (pos >= m_tokens.size())
        throw std::runtime_error("New token stream position is beyond its limits");

    m_currentPos = pos;
}
