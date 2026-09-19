#include "Diagnostics/DiagnosticBuilder.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "SourceManager/SourceManager.h"

/**
 * Binds this builder to a collector and initializes the owned message using the collector's memory resource.
 */
DiagnosticBuilder::DiagnosticBuilder(class DiagnosticCollector *collector) :
    m_collector(collector), m_message(collector->getAllocator())
{
}

/**
 * Convenience constructor that additionally stamps the message type and sender via build().
 */
DiagnosticBuilder::DiagnosticBuilder(class DiagnosticCollector *collector,
                                     DiagnosticMessageType type,
                                     const std::string_view &sender) : DiagnosticBuilder(collector)
{
    build(type, sender);
}

/**
 * Transfers ownership of the pending message from other; nulls other's collector so its
 * destructor becomes a no-op instead of flushing the moved-from message.
 */
DiagnosticBuilder::DiagnosticBuilder(DiagnosticBuilder &&other) :
    m_collector(other.m_collector), m_message(std::move(other.m_message))
{
    // Disconnect the old builder so its destructor doesn't flush an empty message
    other.m_collector = nullptr;
}

/**
 * Flushes any pending message to the collector on scope exit.
 */
DiagnosticBuilder::~DiagnosticBuilder() { flush(); }

/**
 * Appends a note associated with a source reference by delegating to the raw append path.
 */
DiagnosticBuilder &DiagnosticBuilder::appendNote(class SourceReference *sourceRef, std::string_view message)
{
    return appendNoteRaw(message, sourceRef);
}

/**
 * Appends a note with no associated source reference.
 */
DiagnosticBuilder &DiagnosticBuilder::appendNote(std::string_view message) { return appendNoteRaw(message, nullptr); }

/**
 * Records the severity and emitting component for the pending message and returns this builder
 * to support fluent chaining.
 */
DiagnosticBuilder &DiagnosticBuilder::build(DiagnosticMessageType type, const std::string_view &sender)
{
    m_message.setType(type);
    m_message.setSender(sender);

    return *this;
}

/**
 * Streams a string into the main message body.
 */
DiagnosticBuilder &DiagnosticBuilder::operator<<(const std::string_view &str)
{
    m_message.addMainMsg(str);
    return *this;
}

/**
 * Streams a source reference, making it the primary span of the main message.
 */
DiagnosticBuilder &DiagnosticBuilder::operator<<(class SourceReference *sourceRef)
{
    m_message.setPrimarySourceRef(sourceRef);
    return *this;
}

/**
 * Publishes the pending message to the collector exactly once and detaches from it so the
 * destructor does not emit again.
 */
void DiagnosticBuilder::flush()
{
    if (m_collector)
    {
        m_collector->onDiag(std::move(m_message));
        m_collector = nullptr;
    }
}

/**
 * Forwards the enablement query to the collector, letting callers skip formatting disabled diagnostics.
 */
bool DiagnosticBuilder::isDiagEnabledForType(DiagnosticMessageType type) const
{
    return m_collector->isDiagEnabledForType(type);
}

/**
 * Copies the note text into the collector's arena and attaches it with the given source reference.
 */
DiagnosticBuilder &DiagnosticBuilder::appendNoteRaw(std::string_view str, class SourceReference *ref)
{
    std::pmr::string copyMsg(m_collector->getAllocator());
    copyMsg += str;
    m_message.addNote({ ref, copyMsg });

    return *this;
}