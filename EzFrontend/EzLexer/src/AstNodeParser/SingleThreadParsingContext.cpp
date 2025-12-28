#include "AstNodeParser/SingleThreadParsingContext.h"

SingleThreadParsingContext::SingleThreadParsingContext(std::shared_ptr<ErrorCollector> errorCollector,
                                                       std::shared_ptr<SourceManager> sourceManager,
                                                       std::vector<TokenInformation> tokens) :
    m_currentPos(0), m_errorCollector(std::move(errorCollector)), m_sourceManager(std::move(sourceManager)),
    m_tokens(std::move(tokens))
{
}

bool SingleThreadParsingContext::canPeek() const { return m_currentPos < m_tokens.size(); }

size_t SingleThreadParsingContext::getCurrentPosition() const { return m_currentPos; }

size_t SingleThreadParsingContext::getRemainingTokenCount() const
{
    if (m_tokens.size() > m_currentPos)
        return m_tokens.size() - m_currentPos;
    else
        return 0;
}

const TokenInformation &SingleThreadParsingContext::peek() const { return m_tokens.data()[getCurrentPosition()]; }

void SingleThreadParsingContext::beginMultiSourceRef() { m_multiSourceRefs.push({}); }

void SingleThreadParsingContext::consume()
{
    if (canPeek())
    {
        m_multiSourceRefs.top().push_back(m_tokens[m_currentPos].m_sourceReference);
        m_currentPos++;

        if (canPeek() && peek().m_type == _TokenType::Comment)
        {
            // If there's a comment, we skip it.
            consume();
        }
    }
}

void SingleThreadParsingContext::endMultiSourceRef() { m_multiSourceRefs.pop(); }

void SingleThreadParsingContext::setPosition(size_t pos)
{
    if (pos >= m_tokens.size())
        throw std::runtime_error("New token stream position is beyond its limits");

    m_currentPos = pos;
}

const std::shared_ptr<ErrorCollector> &SingleThreadParsingContext::getErrorCollector() const
{
    return m_errorCollector;
}

const std::shared_ptr<SourceManager> &SingleThreadParsingContext::getSourceManager() const { return m_sourceManager; }

std::vector<std::shared_ptr<SourceReference>> SingleThreadParsingContext::getCurrentMultiReference() const
{
    if (!m_multiSourceRefs.empty())
        return m_multiSourceRefs.top();

    return {};
}
