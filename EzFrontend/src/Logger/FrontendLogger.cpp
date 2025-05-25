#include "Logger/FrontendLogger.h"

FrontendLogger::FrontendLogger(const std::shared_ptr<SourceManager> &sourceManager) : m_sourceManager(sourceManager)
{
}
void FrontendLogger::logError(const LogMessage &msg)
{
    g_logger->logError(msg);
}

void FrontendLogger::logError(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("FRONTEND")
                             .add("Error detected: ")
                             .add(msg.getRawMessage())
                             .add("\n{}, line {}: -> \n\t{}\n", sourceRef->m_sourceFile, sourceRef->m_line,
                                  m_sourceManager->getReferenceContent(sourceRef));
    g_logger->logError(fullMsg);
}

void FrontendLogger::logMessage(const LogMessage &msg)
{
    g_logger->logInfo(msg);
}

void FrontendLogger::logMessage(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("FRONTEND")
                             .add("Message: ")
                             .add(msg.getRawMessage())
                             .add("\n{}, line{}: -> \n\t{}\n", sourceRef->m_sourceFile, sourceRef->m_line,
                                  m_sourceManager->getReferenceContent(sourceRef));
    g_logger->logInfo(fullMsg);
}

void FrontendLogger::logWarn(const LogMessage &msg)
{
    g_logger->logWarn(msg);
}

void FrontendLogger::logWarn(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef)
{
    LogMessage fullMsg = LogMessage("FRONTEND")
                             .add("Warning detected: ")
                             .add(msg.getRawMessage())
                             .add("\n\tThrown by: {}", m_sourceManager->getReferenceContent(sourceRef));
    g_logger->logWarn(fullMsg);
}
