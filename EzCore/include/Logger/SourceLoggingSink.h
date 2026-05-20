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
     * @brief Creates the sink with the given logger.
     * @param logger The logger to use for this sink.
     */
    SourceLoggingSink(ILogger *logger);

    /**
     * @brief Logs a given error message, adding source reference.
     * @param msg The log message.
     * @param sourceManager The source manager to resolve the source reference.
     * @param sourceRef The reference to the source code.
     */
    virtual void logSourceError(LogMessage msg,
                                const std::shared_ptr<SourceManager> &sourceManager,
                                const SourceReference &sourceRef);

    /**
     * @brief Logs a given message, adding source reference.
     * @param msg The log message.
     * @param sourceManager The source manager to resolve the source reference.
     * @param sourceRef The reference to the source code.
     */
    virtual void logSourceMessage(LogMessage msg,
                                  const std::shared_ptr<SourceManager> &sourceManager,
                                  const SourceReference &sourceRef);

    /**
     * @brief Logs a given warning message, adding source reference.
     * @param msg The log message.
     * @param sourceManager The source manager to resolve the source reference.
     * @param sourceRef The reference to the source code.
     */
    virtual void logSourceWarning(LogMessage msg,
                                  const std::shared_ptr<SourceManager> &sourceManager,
                                  const SourceReference &sourceRef);

  private:
};

#endif // EZPACKER_SOURCELOGGINGSINK_H
