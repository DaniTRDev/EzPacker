#ifndef EZPACKER_ERRORCOLLECTOR_H
#define EZPACKER_ERRORCOLLECTOR_H

#include "EzFrontendCommon.h"
#include "Logger/FrontendLogger.h"

enum class ErrorHandleType
{
    Commit, // Shows the errors.
    Discard, // Removes all errors of this scope
    Propagate // Copies errors into upper scope.
};

/**
 * This class represents a collector of errors used by the parser. It's required because parsing might throw errors that
 * are not really errors. Here's an example:
 *
 * Imagine we have an optional rule A, composed by a sequence ({C, B}). If C or B fails, an error will be thrown, but
 * since the parent rule was optional in the first place, these errors MUST be discarded.
 */
class ErrorCollector
{
  public:
    /**
     * Creates the collector with the given logger and source manager.
     * @param logger
     * @param sourceManager
     */
    ErrorCollector(const std::shared_ptr<FrontendLogger> &logger, const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Destroys the object and free resources.
     */
    ~ErrorCollector();

    /**
     * Enters in the scope of a new rule. It will push a new element to the stack of errors.
     */
    void enterRule();

    /**
     * Exits the current scope of the rule.
     * If handle == Commit -> Errors will be printed.
     * If handle == Discard -> Errors will be discarded.
     * If handle == Propagate -> Errors will be copies into upper scope. If there's no scope, errors will be shown.
     * @param handle
     */
    void exitRule(ErrorHandleType handle);

    /**
     * Adds an error to the current scope. If no scope is set, a new scope will be created.
     * @param msg
     */
    void collect(const LogMessage &msg);

    /**
     * Adds an error to the current scope that is caused by the given source reference. If no scope is set, a new scope
     * will be created. If no scope is set, a new one will be created.
     * @param msg
     * @param ref
     */
    void collect(const LogMessage &msg, const std::shared_ptr<SourceReference> &ref);

  private:
    std::shared_ptr<FrontendLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    // Each rule pushes a new element in the stack when it's going to be matched and pops it when a result have been
    // calculated. Each "scope" has a set of errors in which the order matters.
    std::stack<std::queue<LogMessage>> m_errors;
};

#endif // EZPACKER_ERRORCOLLECTOR_H
