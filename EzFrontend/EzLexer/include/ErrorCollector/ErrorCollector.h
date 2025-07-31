#ifndef EZPACKER_ERRORCOLLECTOR_H
#define EZPACKER_ERRORCOLLECTOR_H

#include "EzLexerCommon.h"
#include "Logger/SourceLoggingSink.h"

/**
 * Enum that sets how we should handle the errors at the current scope.
 */
enum class ErrorHandleType
{
    Commit,   // Show ALL the errors.
    Discard,  // Removes ALL the errors of this scope
    Propagate // Copies errors into UPPER scope.
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
     * Creates the collector with the given log sink and source manager.
     * @param logSink
     * @param sourceManager
     */
    ErrorCollector(const std::shared_ptr<SourceLoggingSink> &logSink,
                   const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Destroys the object and free resources.
     */
    ~ErrorCollector();

    /**
     * Enters in a new scope. It will push a new element to the stack of scopes.
     */
    void enterScope();

    /**
     * Exits the current scope and proceeds as "handle" tells the function to. See ErrorHandleType for more information.
     * If there's no scope, an error is thrown.
     * @param handle
     */
    void exitScope(ErrorHandleType handle);

    /**
     * Adds an error to the current scope. If no scope is set, error will be printed right away.
     * @param msg
     */
    void error(LogMessage msg);

    /**
     * Adds an error to the current scope that is caused by the given source reference. If no scope is set, a new one
     * will be created.
     * @param msg
     * @param ref
     */
    void error(LogMessage msg, const std::shared_ptr<SourceReference> &ref);

  private:
    std::shared_ptr<SourceLoggingSink> m_logSink;
    std::shared_ptr<SourceManager> m_sourceManager;
    /*
     * Each rule pushes a new element in the stack when it's going to be matched and pops it when a result has been
     * calculated. Each "scope" has a set of errors in which the order matters.
     */
    std::stack<std::queue<LogMessage>> m_errors;
};

#endif // EZPACKER_ERRORCOLLECTOR_H
