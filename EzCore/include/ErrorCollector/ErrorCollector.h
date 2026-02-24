#ifndef EZPACKER_ERRORCOLLECTOR_H
#define EZPACKER_ERRORCOLLECTOR_H

#include "EzCoreCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * Enum that sets how we should handle the errors at the current end-of-scope.
 */
enum class ErrorAction
{
    Commit,   // Sends ALL the errors of this scope to the subscriber.
    Discard,  // Removes ALL the errors of this scope
    Propagate // Copies errors into UPPER scope.
};

enum class ErrorSeverity : uint8_t
{
    NoError = 0, // It's just an information message.
    Warning = 1, // Not really an error, just something "weird" that the developer may want to know.

    Soft = 2, /*
               * A parser wasn't able to identify the AstNode.
               */

    Fatal = 3 /*
               * A parser has identified that what's parsing is really its AstNode but it's malformed.
               */
};

struct Error
{
    ErrorSeverity m_severity;
    std::shared_ptr<SourceReference> m_sourceRef; // Can be nullptr
    std::string m_message;
    std::string m_sender;    /* Module that threw the error.
                              * TODO: Change this for a modular system in which module add themselves (lexer, parser,
                              * TODO: semantic, instr lowerer, ...)
                              */
    std::string m_timeStamp; // Set in the creation of this structure.
};

/**
 * This structure allows to control how a subscriber attaches / detaches itself from the error flow.
 */
struct ErrorCollectorSubscriber
{
    using CallbackType = void(void *userParam, const std::shared_ptr<Error> &error);
    bool m_listening;
    CallbackType *m_callback;
    void *m_param;
};

struct ErrorScope
{
    bool m_hasFatalError;
    std::stack<std::shared_ptr<Error>> m_errors;
};

/**
 * This class represents a collector of errors used by both low-level and high-level modules. It provides a
 * simple, yet extensive API to hook into error flow.
 */
class ErrorCollector
{
  public:
    /**
     * Returns true if the current scope hast at least 1 fatal error. If there's no scope, false is returned.
     * @return bool
     */
    bool doesCurrentScopeHasFatalErrors();

    /**
     * Adds a subscriber to the collector. Returns the ID of the collector.
     * @param callback
     * @param param
     * @return size_t
     */
    size_t addSubscriber(ErrorCollectorSubscriber::CallbackType *callback, void *param);

    /**
     * Begins a scope of errors.
     */
    void beginScope();

    /**
     * Ends the scope and executes the given action. Look at 'ErrorAction' to see what each action does. If there isn't
     * any scope, an exception is thrown. If the current scope is the top-most scope, errors will be COMMITED.
     * @param action
     */
    void endScope(ErrorAction action);

    /**
     * Adds an error to the current scope. If no scope has been begun, an exception is thrown.
     * @param severity
     * @param message
     * @param sender
     * @param sourceRef
     */
    void onError(ErrorSeverity severity,
                 const std::string &message,
                 const std::string &sender,
                 const std::shared_ptr<SourceReference> &sourceRef = nullptr);

  private:
    std::recursive_mutex m_mutex;

    /**
     * Each scope can define sub-scopes, ... When a scope ends it must define an action to handle errors.
     */
    std::stack<std::shared_ptr<ErrorScope>> m_errorStack;
    std::vector<std::unique_ptr<ErrorCollectorSubscriber>> m_subscribers;
};

#endif // EZPACKER_ERRORCOLLECTOR_H
