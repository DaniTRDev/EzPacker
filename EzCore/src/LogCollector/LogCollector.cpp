#include "LogCollector/LogCollector.h"

std::unique_ptr<Logger> LogCollector::m_logger = nullptr;

void LogCollector::log(const LogMessage &msg, const std::string_view &sender)
{
    if (!m_logger)
    {
        m_logger = EzLogger::createSyncLogger("EzPacker");
    }

    g_logger->pushLog(LogMessage("[{}] -> ", sender).add(msg));
}

void LogCollector::logFromSourceRef(const LogMessage &msg,
                                    SourceManager *sourceManager,
                                    const SourceReference &ref,
                                    const std::string_view &sender)
{
    if (ref.m_valid)
    {
        log(LogMessage("{}:{}:{}\n\t ", sourceManager->getSourceName(ref.m_sourceFileId), ref.m_line, ref.m_col)
                    .add(msg)
                    .add(" at\n \t{}", sourceManager->getReferenceContent(ref)),
            sender);
    }
}
