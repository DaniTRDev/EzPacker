#include "ErrorCollector/ErrorEmitter.h"

ErrorEmitter::ErrorEmitter(const std::shared_ptr<ErrorCollector> &errorCollector,
                           const std::shared_ptr<SourceManager> &sourceManager) :
    m_errorCollector(errorCollector), m_sourceManager(sourceManager)
{
}

void ErrorEmitter::emitError(ErrorSeverity severity,
                             const std::string &message,
                             const std::string &sender,
                             const std::shared_ptr<SourceReference> &sourceRef)
{
    m_errorCollector->onError(severity, message, sender, sourceRef);
}

const std::shared_ptr<ErrorCollector> &ErrorEmitter::getErrorCollector() const { return m_errorCollector; }

const std::shared_ptr<SourceManager> &ErrorEmitter::getSourceManager() const { return m_sourceManager; }
