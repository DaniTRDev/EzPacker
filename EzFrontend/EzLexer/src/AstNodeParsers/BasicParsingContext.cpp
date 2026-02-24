#include "AstNodeParsers/BasicParsingContext.h"

BasicParsingContext::BasicParsingContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                         const std::shared_ptr<SourceManager> &sourceManager,
                                         std::vector<TokenInformation> tokens) :
    m_currentPos(0), m_tokens(std::move(tokens)), ErrorEmitter(errorCollector, sourceManager)
{
}

bool BasicParsingContext::canPeek() const { return m_currentPos < m_tokens.size(); }

size_t BasicParsingContext::getCurrentPosition() const { return m_currentPos; }

size_t BasicParsingContext::getRemainingTokenCount() const
{
    if (m_tokens.size() > m_currentPos)
        return m_tokens.size() - m_currentPos;
    else
        return 0;
}

const TokenInformation &BasicParsingContext::peek() const { return m_tokens.data()[getCurrentPosition()]; }

void BasicParsingContext::beginMultiSourceRef() { m_multiSourceRefs.push({}); }

void BasicParsingContext::consume()
{
    if (canPeek())
    {
        m_lastSourceRef = m_tokens[m_currentPos].m_sourceReference;
        m_multiSourceRefs.top().push_back(m_lastSourceRef);
        m_currentPos++;

        if (canPeek() && peek().m_type == _TokenType::Comment)
        {
            // If there's a comment, we skip it.
            consume();
        }
    }
}

void BasicParsingContext::endMultiSourceRef() { m_multiSourceRefs.pop(); }

void BasicParsingContext::setPosition(size_t pos)
{
    if (pos >= m_tokens.size())
        throw std::runtime_error("New token stream position is beyond its limits");

    m_currentPos = pos;
}

const std::shared_ptr<SourceReference> &BasicParsingContext::getLastSourceReference() const { return m_lastSourceRef; }

std::vector<std::shared_ptr<SourceReference>> BasicParsingContext::getCurrentMultiReference() const
{
    if (!m_multiSourceRefs.empty())
        return m_multiSourceRefs.top();

    return {};
}
