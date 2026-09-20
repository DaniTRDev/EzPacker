#ifndef EZCORE_DIAGNOSTIC_BUILDER_H
#define EZCORE_DIAGNOSTIC_BUILDER_H

#include "EzCoreCommon.h"
#include "DiagnosticMessage.h"

/**
 * This class is not thread-safe, each instance of DiagnosticBuilder must be used by 1 single thread.
 */
class DiagnosticBuilder
{
  public:
    // Delete copy operations to avoid duplicate error emissions
    DiagnosticBuilder(const DiagnosticBuilder &) = delete;
    DiagnosticBuilder &operator=(const DiagnosticBuilder &) = delete;

    /**
     * Creates the builder and attaches it to a collector.
     */
    DiagnosticBuilder(class DiagnosticCollector *collector);

    /**
     * Creates the builder, builds a simple diagnostic message and attaches it to a collector.
     */
    DiagnosticBuilder(class DiagnosticCollector *collector, DiagnosticMessageType type, std::string_view sender);

    /**
     * Defines the move constructor. It will unlink other's from emitting the message.
     */
    DiagnosticBuilder(DiagnosticBuilder &&other);

    /**
     * When this object is destroyed, the message is flushed into the collector.
     */
    ~DiagnosticBuilder();

    /**
     * Utility function that builds a note with the given ref and format str. If the current's message type level log is
     * not enabled, the function won't even format the string and will return early.
     */
    template <typename... Args>
    DiagnosticBuilder &appendNote(class SourceReference *ref, std::format_string<Args...> fmt, Args &&...args)
    {
        if (!isDiagEnabledForType(m_message.getType()))
        {
            return *this; // Exit immediately. To avoid allocations.
        }

        // If we got here, the message can be notified to the collector. Format and send to the appendNote method.
        return appendNoteRaw(std::format(fmt, std::forward<Args>(args)...), ref);
    }

    /**
     * Utility function that builds a note with the given format str. If the current's message type level log is
     * not enabled, the function won't even format the string and will return early.
     */
    template <typename... Args> DiagnosticBuilder &appendNote(std::format_string<Args...> fmt, Args &&...args)
    {
        if (!isDiagEnabledForType(m_message.getType()))
        {
            return *this; // Exit immediately. To avoid allocations.
        }

        // If we got here, the message can be notified to the collector. Format and send to the appendNote method.
        return appendNoteRaw(std::format(fmt, std::forward<Args>(args)...), nullptr);
    }

    /**
     * Appends a note with the given source reference and message.
     */
    DiagnosticBuilder &appendNote(class SourceReference *sourceRef, std::string_view message);

    /**
     * Appends a note with the given message and WITHOUT a source reference.
     */
    DiagnosticBuilder &appendNote(std::string_view message);

    /**
     * Sets the type and sender of the current message.
     */
    DiagnosticBuilder &build(DiagnosticMessageType type, std::string_view sender);

    /**
     * Operator used to append a string into the main message.
     */
    DiagnosticBuilder &operator<<(std::string_view str);

    /**
     * Operator used to append a source reference to the main message.
     */
    DiagnosticBuilder &operator<<(class SourceReference *sourceRef);

    /**
     * Pushes the current message to the diagnostic collector and clears it.
     */
    void flush();

  private:
    /**
     * Simple wrapper that calls m_collector method. Made a whole new method just not to make the builder require
     * including collector's files.
     */
    bool isDiagEnabledForType(DiagnosticMessageType type) const;

    /**
     * Internal append method used to stop template recursion in the template appends.
     */
    DiagnosticBuilder &appendNoteRaw(std::string_view str, class SourceReference *ref);

  private:
    class DiagnosticCollector *m_collector; // Collector that receives the message on flush; null once flushed/moved.
    DiagnosticMessage m_message;            // The pending message being assembled by this builder.
};

#endif // EZCORE_DIAGNOSTIC_BUILDER_H
