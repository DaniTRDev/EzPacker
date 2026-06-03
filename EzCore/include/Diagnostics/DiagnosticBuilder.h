#ifndef EZPACKER_DIAGNOSTICBUILDER_H
#define EZPACKER_DIAGNOSTICBUILDER_H

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
     * @param collector
     */
    DiagnosticBuilder(class DiagnosticCollector *collector);

    /**
     * Creates the builder, builds a simple diagnostic message and attaches it to a collector.
     * @param collector
     */
    DiagnosticBuilder(class DiagnosticCollector *collector, DiagnosticMessageType type, const std::pmr::string &sender);

    /**
     * When this object is destroyed, the message is flushed into the collector.
     */
    ~DiagnosticBuilder();

    /**
     * Appends a note to the current message.
     * @param message
     * @param loc
     * @return
     */
    DiagnosticBuilder &appendNote(const std::pmr::string &message, SourceReference *sourceRef);

    /**
     * Sets the type and sender of the current message.
     * @param type
     * @param sender
     * @return
     */
    DiagnosticBuilder &build(DiagnosticMessageType type, const std::pmr::string &sender);

    /**
     * Operator used to append a string into the main message.
     * @param message
     * @return
     */
    DiagnosticBuilder &operator<<(const std::pmr::string &str);

    /**
     *
     * @param sourceRef
     * @param str
     * @return
     */
    DiagnosticBuilder &operator<<(SourceReference *sourceRef);

    /**
     * Pushes the current message to the diagnostic collector and clears it.
     */
    void flush();

  private:
    class DiagnosticCollector *m_collector;
    DiagnosticMessage m_message;
};

#endif // EZPACKER_DIAGNOSTICBUILDER_H
