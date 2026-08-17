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
    DiagnosticBuilder(class DiagnosticCollector *collector, DiagnosticMessageType type, const std::string_view &sender);

    /**
     * When this object is destroyed, the message is flushed into the collector.
     */
    ~DiagnosticBuilder();

    /**
     * Appends a note to the current message.
     */
    DiagnosticBuilder &appendNote(const std::pmr::string &message, class SourceReference *sourceRef = nullptr);

    /**
     * Sets the type and sender of the current message.
     */
    DiagnosticBuilder &build(DiagnosticMessageType type, const std::string_view &sender);

    /**
     * Operator used to append a string into the main message.
     */
    DiagnosticBuilder &operator<<(const std::pmr::string &str);

    /**
     * Operator used to append a source reference to the main message.
     */
    DiagnosticBuilder &operator<<(class SourceReference *sourceRef);

    /**
     * Pushes the current message to the diagnostic collector and clears it.
     */
    void flush();

  private:
    class DiagnosticCollector *m_collector;
    DiagnosticMessage m_message;
};

#endif // EZCORE_DIAGNOSTIC_BUILDER_H
