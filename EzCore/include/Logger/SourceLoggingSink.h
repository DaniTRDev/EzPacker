#ifndef EZPACKER_SOURCELOGGINGSINK_H
#define EZPACKER_SOURCELOGGINGSINK_H

#include "EzCoreCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * This class represents a common entry point for every module of the grammar when it comes to logging.
 */
class SourceLoggingSink : public LogSink
{
  public:
    /**
     * Creates the sink with the given logger and source manager.
     * @param logger
     * @param sourceManager
     */
    SourceLoggingSink(ILogger *logger, const std::shared_ptr<SourceManager> &sourceManager);
    

    /**
     * Logs a given error message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logSourceError(LogMessage msg, const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs a given message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logSourceMessage(LogMessage msg, const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs a given warning message, adding source reference.
     * @param msg
     * @param sourceRef
     */
    void logSourceWarning(LogMessage msg, const std::shared_ptr<SourceReference> &sourceRef);

  private:
    std::shared_ptr<SourceManager> m_sourceManager;
};

#endif // EZPACKER_SOURCELOGGINGSINK_H
