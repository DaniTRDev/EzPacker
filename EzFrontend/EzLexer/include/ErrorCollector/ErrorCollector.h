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

using OnError = std::function<void(LogMessage msg)>;
using OnInfoType = std::function<void(LogMessage msg)>;

struct ErrorCollectorPipe
{
    OnError m_onErrorCallback;
    OnInfoType m_onInfoCallback;
};

/**
 * This class represents a collector of errors used by the parser and other high-level modules. It provides a
 * simple API to hook into messages.
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
     * Returns true if the is at least 1 error in the collector.
     * @return bool
     */
    bool areThereErrors() const;

    /**
     * Adds a pipe to the pipe list. Each pipe must define 1 callback for each message type (error, info) and will
     * be called with an object of LogMessage, which is a COPY for the pipe to modify on its own.
     * @param pipe
     */
    void addPipe(ErrorCollectorPipe pipe);

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
     * Adds an error to the current scope that is caused by the given source reference. If no scope is set, a new one
     * will be created.
     * @param msg
     * @param ref
     */
    void error(LogMessage msg, const std::shared_ptr<SourceReference> &ref);

    /**
     * Adds an information message (not an error). It doesn't need to be run inside a scope.
     * @param msg
     */
    void information(LogMessage msg);

  private:
    std::shared_ptr<SourceLoggingSink> m_logSink;
    std::shared_ptr<SourceManager> m_sourceManager;
    /*
     * Each rule pushes a new element in the stack when it's going to be matched and pops it when a result has been
     * calculated. Each "scope" has a set of errors in which the order matters.
     */
    std::stack<std::stack<LogMessage>> m_errors;
    /**
     * List of pipes that are listening for messages on the error collector.
     */
    std::vector<ErrorCollectorPipe> m_pipes;
};
#endif // EZPACKER_ERRORCOLLECTOR_H
