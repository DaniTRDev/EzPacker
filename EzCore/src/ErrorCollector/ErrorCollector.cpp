#include "ErrorCollector/ErrorCollector.h"

bool ErrorCollector::doesCurrentScopeHasFatalErrors()
{
    std::scoped_lock lock(m_mutex);

    if (m_errorStack.empty())
        return false;

    return m_errorStack.top()->m_hasFatalError;
}

size_t ErrorCollector::addSubscriber(ErrorCollectorSubscriber::CallbackType *callback, void *param)
{
    std::scoped_lock lock(m_mutex);

    size_t id = m_subscribers.size();
    std::unique_ptr<ErrorCollectorSubscriber> subscriber = std::make_unique<ErrorCollectorSubscriber>(
            ErrorCollectorSubscriber{ .m_listening = true, .m_callback = callback, .m_param = param });

    m_subscribers.push_back(std::move(subscriber));
    return id;
}

void ErrorCollector::beginScope()
{
    std::scoped_lock lock(m_mutex);
    m_errorStack.emplace(std::make_shared<ErrorScope>(ErrorScope{ .m_hasFatalError = false, .m_errors = {} }));
}

void ErrorCollector::endScope(ErrorAction action)
{
    std::scoped_lock lock(m_mutex);

    if (m_errorStack.empty())
        throw std::runtime_error("There is no scope to end, call beginScope()");

    switch (action)
    {
        case ErrorAction::Commit:
        {
            std::shared_ptr<ErrorScope> scope = m_errorStack.top();
            m_errorStack.pop();

            std::list<std::shared_ptr<Error>> errors;
            while (!scope->m_errors.empty())
            {
                std::shared_ptr<Error> error = scope->m_errors.top();
                scope->m_errors.pop();

                errors.push_front(std::move(error));
            }

            for (auto &error : errors)
            {
                for (auto &subscriber : m_subscribers)
                {
                    if (subscriber->m_listening)
                    {
                        subscriber->m_callback(subscriber->m_param, error);
                    }
                }
            }

            break;
        }
        case ErrorAction::Discard:
        {
            m_errorStack.pop();
            break;
        }
        case ErrorAction::Propagate:
        {
            if (m_errorStack.size() == 1)
            {
                return endScope(ErrorAction::Commit);
            }

            std::shared_ptr<ErrorScope> currentScope = m_errorStack.top();
            m_errorStack.pop();

            std::vector<std::shared_ptr<Error>> tempStorage;
            while (!currentScope->m_errors.empty())
            {
                tempStorage.push_back(currentScope->m_errors.top());
                currentScope->m_errors.pop();
            }

            for (auto it = tempStorage.rbegin(); it != tempStorage.rend(); ++it)
            {
                m_errorStack.top()->m_errors.push(std::move(*it));
            }

            m_errorStack.top()->m_hasFatalError |= currentScope->m_hasFatalError;
            break;
        }
    }
}

void ErrorCollector::onError(ErrorSeverity severity,
                             const std::string &message,
                             const std::string &sender,
                             const SourceReference &sourceRef)
{
    std::scoped_lock lock(m_mutex);

    if (m_errorStack.empty())
        throw std::runtime_error("There is no scope to add error to, call beginScope()");

    auto now = std::chrono::system_clock::now();
    auto timePoint = std::chrono::floor<std::chrono::seconds>(now);

    if (severity == ErrorSeverity::Fatal)
        m_errorStack.top()->m_hasFatalError = true;

    m_errorStack.top()->m_errors.push(std::make_shared<Error>(Error{
            .m_severity = severity,
            .m_sourceRef = sourceRef,
            .m_message = message,
            .m_sender = sender,
            .m_timeStamp = std::format("{:%H:%M:%S}", timePoint),
    }));
}
