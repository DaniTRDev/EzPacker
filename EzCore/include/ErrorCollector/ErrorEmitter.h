#ifndef EZPACKER_ERROREMITTER_H
#define EZPACKER_ERROREMITTER_H

#include "EzCoreCommon.h"
#include "ErrorCollector.h"

/**
 * Basic class of which other classes can inherit from to acquire the capacity of emitting errors into an error
 * collector.
 */
class ErrorEmitter
{
  public:
    /**
     * Creates the emitter with the given collector and source manager.
     * @param errorCollector
     * @param sourceManager
     */
    ErrorEmitter(const std::shared_ptr<ErrorCollector> &errorCollector,
                 const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Adds an error to the error collector. By default it just calls ErrorCollector::onError.
     * Derived classes can extend this functionality.
     * @param severity
     * @param message
     * @param sender
     * @param sourceRef
     */
    virtual void emitError(ErrorSeverity severity,
                           const std::string &message,
                           const std::string &sender,
                           const std::shared_ptr<SourceReference> &sourceRef = nullptr);

    /**
     * Returns the error collector.
     * @return const std::shared_ptr<ErrorCollector> &
     */
    const std::shared_ptr<ErrorCollector> &getErrorCollector() const;

    /**
     * Returns the source manager.
     * @return const std::shared_ptr<SourceManager> &
     */
    const std::shared_ptr<SourceManager> &getSourceManager() const;

  private:
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
};

#endif // EZPACKER_ERROREMITTER_H
