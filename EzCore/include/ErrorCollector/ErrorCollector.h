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
    Warning = 1, // Not really an error, just something "weird" that the developer may want to know.

    Soft = 2, /*
               * An error that will be shown if a fatal error is risen (as a TRACE).
               */

    Fatal = 3 /*
               * A component reported an error that must abort the execution.
               */
};

struct Error
{
    ErrorSeverity m_severity;
    SourceReference m_sourceRef;
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
     * @brief Checks if the current scope has at least one fatal error.
     * @returns True if the current scope has fatal errors, false otherwise.
     */
    bool doesCurrentScopeHasFatalErrors();

    /**
     * @brief Adds a subscriber to the collector.
     * @param callback The callback function to be called when an error is committed.
     * @param param A user-defined parameter to be passed to the callback.
     * @returns The ID of the subscriber.
     */
    size_t addSubscriber(ErrorCollectorSubscriber::CallbackType *callback, void *param);

    /**
     * @brief Begins a new scope for errors.
     */
    void beginScope();

    /**
     * @brief Ends the current scope and executes the given action.
     * @param action The action to perform on the errors in the current scope.
     */
    void endScope(ErrorAction action);

    /**
     * @brief Adds an error to the current scope.
     * @param severity The severity of the error.
     * @param message The error message.
     * @param sender The sender of the error.
     * @param sourceRef A reference to the source code where the error occurred.
     */
    void onError(ErrorSeverity severity,
                 const std::string &message,
                 const std::string &sender,
                 const SourceReference &sourceRef = {});

  private:
    std::recursive_mutex m_mutex;

    /**
     * Each scope can define sub-scopes, ... When a scope ends it must define an action to handle errors.
     */
    std::stack<std::shared_ptr<ErrorScope>> m_errorStack;
    std::vector<std::unique_ptr<ErrorCollectorSubscriber>> m_subscribers;
};

#endif // EZPACKER_ERRORCOLLECTOR_H
