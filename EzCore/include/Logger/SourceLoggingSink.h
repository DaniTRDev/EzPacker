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
     * Creates the sink with the given logger.
     * @param logger
     */
    SourceLoggingSink(ILogger *logger);

    /**
     * Logs a given error message, adding source reference.
     * @param msg
     * @param sourceManager
     * @param sourceRef
     */
    virtual void logSourceError(LogMessage msg,
                                const std::shared_ptr<SourceManager> &sourceManager,
                                const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs a given message, adding source reference.
     * @param msg
     * @param sourceManager
     * @param sourceRef
     */
    virtual void logSourceMessage(LogMessage msg,
                                  const std::shared_ptr<SourceManager> &sourceManager,
                                  const std::shared_ptr<SourceReference> &sourceRef);

    /**
     * Logs a given warning message, adding source reference.
     * @param msg
     * @param sourceManager
     * @param sourceRef
     */
    virtual void logSourceWarning(LogMessage msg,
                                  const std::shared_ptr<SourceManager> &sourceManager,
                                  const std::shared_ptr<SourceReference> &sourceRef);

  private:
};

#endif // EZPACKER_SOURCELOGGINGSINK_H
