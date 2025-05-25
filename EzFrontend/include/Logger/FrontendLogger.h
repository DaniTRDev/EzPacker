#ifndef EZPACKER_FRONTENDLOGGER_H
#define EZPACKER_FRONTENDLOGGER_H

#include "EzFrontendCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * This class represents a common entry point for every module of the frontend when it comes to logging.
 */
class FrontendLogger
{
  public:
    /**
     * Creates the logger with the given source manager.
     * @param sourceManager
     */
    FrontendLogger(const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Logs given error.
     * @param msg
     */
    void logError(const LogMessage &msg);

    /**
     * Logs a given error message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logError(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs given message.
     * @param msg
     */
    void logMessage(const LogMessage &msg);

    /**
     * Logs a given message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logMessage(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs given warning.
     * @param msg
     */
    void logWarn(const LogMessage &msg);

    /**
     * Logs a given warning message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logWarn(const LogMessage &msg, const std::shared_ptr<SourceReference> &sourceRef);

  private:
    std::shared_ptr<SourceManager> m_sourceManager;
};

#endif // EZPACKER_FRONTENDLOGGER_H
