#include "Diagnostics/DiagnosticBuilder.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "SourceManager/SourceManager.h"

DiagnosticBuilder::DiagnosticBuilder(class DiagnosticCollector *collector) :
    m_collector(collector), m_message(collector->getAllocator())
{
}

DiagnosticBuilder::DiagnosticBuilder(class DiagnosticCollector *collector,
                                     DiagnosticMessageType type,
                                     const std::string_view &sender) : DiagnosticBuilder(collector)
{
    build(type, sender);
}

DiagnosticBuilder::DiagnosticBuilder(DiagnosticBuilder &&other) :
    m_collector(other.m_collector), m_message(std::move(other.m_message))
{
    // Disconnect the old builder so its destructor doesn't flush an empty message
    other.m_collector = nullptr;
}

DiagnosticBuilder::~DiagnosticBuilder() { flush(); }

DiagnosticBuilder &DiagnosticBuilder::appendNote(class SourceReference *sourceRef, std::string_view message)
{
    return appendNoteRaw(message, sourceRef);
}

DiagnosticBuilder &DiagnosticBuilder::appendNote(std::string_view message) { return appendNoteRaw(message, nullptr); }

DiagnosticBuilder &DiagnosticBuilder::build(DiagnosticMessageType type, const std::string_view &sender)
{
    m_message.setType(type);
    m_message.setSender(sender);

    return *this;
}

DiagnosticBuilder &DiagnosticBuilder::operator<<(const std::string_view &str)
{
    m_message.addMainMsg(str);
    return *this;
}

DiagnosticBuilder &DiagnosticBuilder::operator<<(class SourceReference *sourceRef)
{
    m_message.setPrimarySourceRef(sourceRef);
    return *this;
}

void DiagnosticBuilder::flush()
{
    if (m_collector)
    {
        m_collector->onDiag(std::move(m_message));
        m_collector = nullptr;
    }
}

bool DiagnosticBuilder::isDiagEnabledForType(DiagnosticMessageType type) const
{
    return m_collector->isDiagEnabledForType(type);
}

DiagnosticBuilder &DiagnosticBuilder::appendNoteRaw(std::string_view str, class SourceReference *ref)
{
    std::pmr::string copyMsg(m_collector->getAllocator());
    copyMsg += str;
    m_message.addNote({ ref, copyMsg });

    return *this;
}