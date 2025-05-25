#include "Parser/ErrorCollector/ErrorCollector.h"

ErrorCollector::ErrorCollector(const std::shared_ptr<FrontendLogger> &logger,
                               const std::shared_ptr<SourceManager> &sourceManager)
    : m_logger(logger), m_sourceManager(sourceManager)
{
}

ErrorCollector::~ErrorCollector()
{
    while (!m_errors.empty())
        m_errors.pop();
}

void ErrorCollector::enterRule()
{
    m_errors.push({});
}

void ErrorCollector::exitRule(ErrorHandleType handle)
{
    if (m_errors.empty())
        throw std::runtime_error("ErrorCollector: Enter was not called previously!");

    if (handle == ErrorHandleType::Discard)
    {
        m_errors.pop();
    }
    else if (handle == ErrorHandleType::Commit)
    {
        auto errors = std::move(m_errors.top());
        while (!errors.empty())
        {
            m_logger->logError(std::move(errors.front()));
            errors.pop();
        }
    }
    else
    {
        // Propagate errors.
        auto errors = std::move(m_errors.top());
        m_errors.pop();

        if (m_errors.empty())
        {
            while (!errors.empty())
            {
                m_logger->logError(std::move(errors.front()));
                errors.pop();
            }
        }
        else
        {
            // If there's a parent scope, copy errors into it.
            while (!errors.empty())
            {
                m_errors.top().push(std::move(errors.front()));
                errors.pop();
            }
        }
    }
}

void ErrorCollector::collect(const LogMessage &msg)
{
    if (m_errors.empty())
        enterRule();

    m_errors.top().push(std::move(msg));
}

void ErrorCollector::collect(const LogMessage &msg, const std::shared_ptr<SourceReference> &ref)
{
    if (m_errors.empty())
        enterRule();

    LogMessage fullMsg = LogMessage("FRONTEND")
                             .add("Parsing error: ")
                             .add(msg.getRawMessage())
                             .add("\n{}, line {}: -> \n\t{}\n", ref->m_sourceFile, ref->m_line,
                                  m_sourceManager->getReferenceContent(ref));
    m_errors.top().push(std::move(fullMsg));
}