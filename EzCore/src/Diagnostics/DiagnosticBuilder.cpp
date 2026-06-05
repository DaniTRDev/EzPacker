#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticBuilder.h"

DiagnosticBuilder::DiagnosticBuilder(struct DiagnosticCollector *collector) : m_collector(collector) {}

DiagnosticBuilder::DiagnosticBuilder(struct DiagnosticCollector *collector,
                                     DiagnosticMessageType type,
                                     const std::string_view &sender) : DiagnosticBuilder(collector)
{
    build(type, sender);
}

DiagnosticBuilder::~DiagnosticBuilder() { flush(); }

DiagnosticBuilder &DiagnosticBuilder::appendNote(const std::pmr::string &message, SourceReference *sourceRef)
{
    m_message.addNote({ sourceRef, message });
    return *this;
}

DiagnosticBuilder &DiagnosticBuilder::build(DiagnosticMessageType type, const std::string_view &sender)
{
    m_message.setType(type);
    m_message.setSender(sender);

    return *this;
}

DiagnosticBuilder &DiagnosticBuilder::operator<<(const std::pmr::string &str)
{
    m_message.addMainMsg(str);
    return *this;
}

DiagnosticBuilder &DiagnosticBuilder::operator<<(SourceReference *sourceRef)
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
