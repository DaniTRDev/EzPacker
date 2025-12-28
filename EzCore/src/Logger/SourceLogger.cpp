#include "Logger/SourceLoggingSink.h"

SourceLoggingSink::SourceLoggingSink(ILogger *logger) : LogSink(logger, LogSegment("")) {}

void SourceLoggingSink::logSourceError(LogMessage msg,
                                       const std::shared_ptr<SourceManager> &sourceManager,
                                       const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("Error detected: ")
                                 .add(msg)
                                 .add("\n{}:{}: -> {}\n",
                                      sourceRef->m_sourceFile,
                                      sourceRef->m_line,
                                      sourceManager->getReferenceContent(sourceRef));
    LogSink::logError(std::move(fullMsg));
}

void SourceLoggingSink::logSourceMessage(LogMessage msg,
                                         const std::shared_ptr<SourceManager> &sourceManager,
                                         const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("Message: ")
                                 .add(msg)
                                 .add("\n{}:{}: -> {}\n",
                                      sourceRef->m_sourceFile,
                                      sourceRef->m_line,
                                      sourceManager->getReferenceContent(sourceRef));
    LogSink::logInfo(std::move(fullMsg));
}

void SourceLoggingSink::logSourceWarning(LogMessage msg,
                                         const std::shared_ptr<SourceManager> &sourceManager,
                                         const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("Warning detected: ")
                                 .add(msg)
                                 .add("\n\tThrown by: {}", sourceManager->getReferenceContent(sourceRef));
    LogSink::logWarn(std::move(fullMsg));
}
