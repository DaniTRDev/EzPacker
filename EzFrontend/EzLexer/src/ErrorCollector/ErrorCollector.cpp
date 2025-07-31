#include "ErrorCollector/ErrorCollector.h"

ErrorCollector::ErrorCollector(const std::shared_ptr<SourceLoggingSink> &logSink,
                               const std::shared_ptr<SourceManager> &sourceManager) :
    m_logSink(logSink), m_sourceManager(sourceManager)
{
}

ErrorCollector::~ErrorCollector()
{
    // Stack doesn't have a direct .clear() method...
    while (!m_errors.empty())
        m_errors.pop();
}

void ErrorCollector::enterScope() { m_errors.emplace(); }

void ErrorCollector::exitScope(ErrorHandleType handle)
{
    if (m_errors.empty())
        throw std::runtime_error("ErrorCollector: Enter was not called previously!");

    switch (handle)
    {
        case ErrorHandleType::Discard:
        {
            m_errors.pop();
            break;
        }
        case ErrorHandleType::Commit:
        {
            while (!m_errors.empty())
            {
                auto errors = std::move(m_errors.top());
                while (!errors.empty())
                {
                    LogMessage msg;
                    errors.front().moveTo(msg);

                    m_logSink->logError(std::move(msg));
                    errors.pop();
                }

                m_errors.pop();
            }
            break;
        }
        case ErrorHandleType::Propagate:
        {
            if (m_errors.size() == 1)
            {
                // If there is no parent scope, commit al errors.
                exitScope(ErrorHandleType::Commit);
            }
            else
            {
                // If there's a parent scope, copy errors into it.
                auto errors = std::move(m_errors.top());
                m_errors.pop();

                while (!errors.empty())
                {
                    m_errors.top().push(std::move(errors.front()));
                    errors.pop();
                }
            }
            break;
        }
    }
}

void ErrorCollector::error(LogMessage msg)
{
    if (m_errors.empty())
        m_logSink->logError(std::move(msg));
    else
        m_errors.top().push(std::move(msg));
}

void ErrorCollector::error(LogMessage msg, const std::shared_ptr<SourceReference> &ref)
{
    LogMessage fullMsg = LogMessage("EzLexer")
                                 .add(msg.getRawMessage())
                                 .add("\n{}:{}: {}: -> \n\t{}\n",
                                      ref->m_sourceFile,
                                      ref->m_line,
                                      ref->m_col,
                                      m_sourceManager->getReferenceContent(ref));
    if (m_errors.empty())
        m_logSink->logError(std::move(fullMsg));
    else
        m_errors.top().push(std::move(fullMsg));
}