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

bool ErrorCollector::areThereErrors() const { return !m_errors.empty(); }

void ErrorCollector::addPipe(ErrorCollectorPipe pipe) { m_pipes.push_back(std::move(pipe)); }

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
                auto errorMessages = std::move(m_errors.top());
                while (!errorMessages.empty())
                {
                    LogMessage msg;
                    auto errorMsg = errorMessages.top();
                    errorMsg.moveTo(msg);

                    for (auto &pipe : m_pipes)
                    {
                        pipe.m_onErrorCallback(msg);
                    }

                    m_logSink->logError(msg);
                    errorMessages.pop();
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
                    m_errors.top().push(std::move(errors.top()));
                    errors.pop();
                }
            }
            break;
        }
    }
}
void ErrorCollector::information(LogMessage msg)
{
    for (auto &pipe : m_pipes)
    {
        pipe.m_onInfoCallback(msg);
    }

    m_logSink->logInfo(std::move(msg));
}

void ErrorCollector::error(LogMessage msg, const std::shared_ptr<SourceReference> &ref)
{
    LogMessage fullMsg = LogMessage("");
    
    if (ref)
    {
        fullMsg.add("{}:{}:{}: -> ", ref->m_sourceFile, ref->m_line + 1, ref->m_col + 1).add(msg);
    }
    else
    {
        fullMsg.add("ERROR: -> ").add(msg);
    }

    if (m_errors.empty())
        enterScope();

    // Errors are handled inside exitScope.
    m_errors.top().push(std::move(fullMsg));
}
