#include "Diagnostics/DiagnosticLogger.h"
#include "SourceManager/SourceManager.h"
#include <algorithm>

/**
 * Creates the source manager lookup and the synchronous EzLogger sink used for output.
 */
DiagnosticLogger::DiagnosticLogger(class SourceManager *sourceManager, std::string outLogPath) :
    m_sourceManager(sourceManager), m_logger(EzLogger::createSyncLogger("EzPacker", outLogPath))
{
}

/**
 * Formats a diagnostic as a single log line: sender, colored severity, main message, optional
 * primary source excerpt and any attached notes, then pushes it to the logger.
 */
void DiagnosticLogger::onDiag(const DiagnosticMessage &msg)
{
    LogMessage log = LogMessage();

    std::string_view senderStr = msg.getSender();
    std::string_view mainMsgStr = msg.getMainMsg();

    log.add("[{}] ", senderStr);
    logType(log, msg.getType());
    log.add(" {}", mainMsgStr);

    if (msg.getPrimarySourceRef())
    {
        log.add("\n");
        logSourceRef(log, msg.getPrimarySourceRef());
    }

    for (const auto &note : msg.getNotes())
    {
        logNote(log, note);
    }

    m_logger->pushLog(std::move(log));
}

/**
 * Appends an indented, magenta "note:" line and, when present, its source reference.
 */
void DiagnosticLogger::logNote(LogMessage &msg, const DiagnosticNote &note)
{
    std::string noteContentStr(note.m_noteContent.begin(), note.m_noteContent.end());

    msg.add("\n\tnote: ").colorize(Colors::magenta);
    msg.add("{}", noteContentStr);

    if (note.m_sourceRef)
    {
        msg.add("\nsource: ");
        logSourceRef(msg, note.m_sourceRef);
    }
}

/**
 * Maps a diagnostic severity to its colored textual prefix (Error/Warning/Trace/Debug).
 */
void DiagnosticLogger::logType(LogMessage &msg, DiagnosticMessageType type)
{
    switch (type)
    {
        case Diag_Error:
        {
            msg.add("Error: ").colorize(Colors::red);
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

/**
 * Renders a source excerpt for the given reference: a blue file:line:column locator, the raw
 * source line indented under a separator, and a green caret/tilde squiggle under the referenced
 * span. Silently does nothing when the manager, reference or line range cannot be resolved.
 */
void DiagnosticLogger::logSourceRef(LogMessage &msg, class SourceReference *sourceRef)
{
    if (!m_sourceManager || !sourceRef)
    {
        return;
    }

    const SourceLineRange *lineRange = m_sourceManager->getReferenceLine(sourceRef);
    if (!lineRange)
    {
        return;
    }

    std::string_view filename = m_sourceManager->getSourceName(sourceRef->m_sourceFileId);
    std::string_view rawLine = m_sourceManager->getRawLineContent(sourceRef);

    // Strip trailing \r if present
    if (!rawLine.empty() && rawLine.back() == '\r')
    {
        rawLine.remove_suffix(1);
    }

    // Calculate column offset (0-based) and 1-based display coordinates
    size_t colOffset = (sourceRef->m_beginOffset >= lineRange->m_beginOffset)
            ? (sourceRef->m_beginOffset - lineRange->m_beginOffset)
            : 0;

    // Output locator: "src/main.c:col:"
    msg.add("{}:{}:{}\n", filename, lineRange->m_lineNumber, colOffset + 1).colorize(Colors::blue);

    // Output the source line
    msg.add("    | {}\n", rawLine);

    // Build whitespace padding, preserving tabs so alignment remains 1:1 with source
    std::string padding(rawLine.substr(0, std::min(colOffset, rawLine.size())));
    std::replace_if(padding.begin(), padding.end(), [](char c) { return c != '\t'; }, ' ');

    // Build squiggle string: '^' for the start token followed by '~' across the length
    size_t refLen = sourceRef->length();
    size_t squiggleLen = (refLen > 0) ? refLen : 1;

    // Clamp squiggles to avoid overflowing past the end of the line
    if (colOffset + squiggleLen > rawLine.size())
    {
        squiggleLen = (rawLine.size() > colOffset) ? (rawLine.size() - colOffset) : 1;
    }

    std::string squiggles = "^";
    if (squiggleLen > 1)
    {
        squiggles.append(squiggleLen - 1, '~');
    }

    // Output squiggle line
    msg.add("    | {}{}\n", padding, squiggles).colorize(Colors::green);
}