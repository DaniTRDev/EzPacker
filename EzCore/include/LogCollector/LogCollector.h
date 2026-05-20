#ifndef EZPACKER_LOGCOLLECTOR_H
#define EZPACKER_LOGCOLLECTOR_H

#include "EzCoreCommon.h"
#include "SourceManager/SourceManager.h"

class LogCollector
{
  public:
    /**
     * @brief Logs a message with the given sender.
     * @param msg The log message.
     * @param sender The sender of the message.
     */
    static void log(const LogMessage &msg, const std::string_view &sender);

    /**
     * @brief Logs a message with a source reference and sender.
     * @param msg The log message.
     * @param sourceManager The source manager to resolve the source reference.
     * @param ref The reference to the source code.
     * @param sender The sender of the message.
     */
    static void logFromSourceRef(const LogMessage &msg,
                                 SourceManager *sourceManager,
                                 const SourceReference &ref,
                                 const std::string_view &sender);

  private:
    static std::unique_ptr<Logger> m_logger;
};

#define LOG_DEBUG(msg, sender) LogCollector::log(LogMessage(std::string(msg)), sender)
#define LOG_DEBUG_WITH_SOURCE(msg, sourceManager, sourceRef, sender)                                                   \
    LogCollector::log(LogMessage(std::string(msg)), sourceManager, sourceRef, sender)

#endif // EZPACKER_LOGCOLLECTOR_H
