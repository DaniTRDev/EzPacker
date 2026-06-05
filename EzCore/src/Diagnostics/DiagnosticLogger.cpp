#include "Diagnostics/DiagnosticLogger.h"

DiagnosticLogger::DiagnosticLogger(SourceManager *sourceManager) :
    m_sourceManager(sourceManager), m_logger(EzLogger::createSyncLogger("EzPacker"))
{
}

void DiagnosticLogger::onDiag(const DiagnosticMessage &msg)
{
    LogMessage log = LogMessage();

    std::string senderStr(msg.getSender().begin(), msg.getSender().end());
    std::string mainMsgStr(msg.getMainMsg().begin(), msg.getMainMsg().end());

    log.add("[{}] ", senderStr);
    logType(log, msg.getType());
    log.add(" {}\n", mainMsgStr);

    if (msg.getPrimarySourceRef())
        logSourceRef(log, *msg.getPrimarySourceRef());

    for (const auto &note : msg.getNotes())
    {
        logNote(log, note);
    }

    m_logger->pushLog(std::move(log));
}

void DiagnosticLogger::logNote(LogMessage &msg, const DiagnosticNote &note)
{
    std::string noteContentStr(note.m_noteContent.begin(), note.m_noteContent.end());

    msg.add("  note: ").colorize(Colors::magenta);
    msg.add("{}", noteContentStr);

    if (note.m_sourceRef && note.m_sourceRef->m_valid)
    {
        logSourceRef(msg, *note.m_sourceRef);
    }
}

void DiagnosticLogger::logType(LogMessage &msg, DiagnosticMessageType type)
{
    switch (type)
    {
        case Diag_Error:
        {
            msg.add("Error: ").colorize(Colors::red); // Assuming your LogSegment uses a LogColor enum
            break;
        }
        case Diag_Warning:
        {
            msg.add("Warning: ").colorize(Colors::yellow);
            break;
        }
        case Diag_Trace:
        {
            msg.add("Trace: ").colorize(Colors::cyan);
            break;
        }
        case Diag_Debug:
        {
            msg.add("Debug: ").colorize(Colors::blue);
            break;
        }
    }
}

void DiagnosticLogger::logSourceRef(LogMessage &msg, const SourceReference &sourceRef)
{
    if (!m_sourceManager || !sourceRef.m_valid)
        return;

    // Retrieve the exact filename line location details
    const auto &filename = m_sourceManager->getSourceName(sourceRef.m_sourceFileId);

    // Output classic file line locator information: "src/main.c:12:5:"
    msg.add(" --> {}:{}:{}\n", filename, sourceRef.m_line + 1, sourceRef.m_col + 1).colorize(Colors::blue);

    // Pull down the string block containing raw textual layouts alongside caret indicators
    const auto &structuralSnippet = m_sourceManager->getReferenceContent(sourceRef);

    // Split text and squiggles so we can colorize the "^~~~" operator selectively
    size_t newlinePos = structuralSnippet.find('\n');
    if (newlinePos != std::string::npos)
    {
        std::string codeLine = structuralSnippet.substr(0, newlinePos);
        std::string caretLine = structuralSnippet.substr(newlinePos + 1);

        msg.add("    " + codeLine + "\n");
        msg.add("    " + caretLine + "\n").colorize(Colors::green); // Highlights the caret line in green
    }
}
