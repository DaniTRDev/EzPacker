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
     * @brief Creates the emitter with the given collector and source manager.
     * @param errorCollector The error collector to emit errors to.
     * @param sourceManager The source manager to resolve source references.
     */
    ErrorEmitter(const std::shared_ptr<ErrorCollector> &errorCollector,
                 const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * @brief Adds an error to the error collector.
     * @param severity The severity of the error.
     * @param message The error message.
     * @param sender The sender of the error.
     * @param sourceRef The reference to the source code.
     */
    virtual void emitError(ErrorSeverity severity,
                           const std::string &message,
                           const std::string &sender,
                           const SourceReference &sourceRef = {});

    /**
     * @brief Returns the error collector.
     * @returns The error collector.
     */
    const std::shared_ptr<ErrorCollector> &getErrorCollector() const;

    /**
     * @brief Returns the source manager.
     * @returns The source manager.
     */
    const std::shared_ptr<SourceManager> &getSourceManager() const;

  private:
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
};

#endif // EZPACKER_ERROREMITTER_H
