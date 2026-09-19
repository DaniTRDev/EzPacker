#ifndef EZCORE_DIAGNOSTIC_LOGGER_H
#define EZCORE_DIAGNOSTIC_LOGGER_H

#include "EzCoreCommon.h"
#include "DiagnosticListener.h"
#include "DiagnosticMessage.h"

/**
 * Wrap between EzLogger and the diagnostic collector.
 */
class DiagnosticLogger : public DiagnosticListener
{
  public:
    /**
     * Creates the logger linked to the given source manager.

     */
    DiagnosticLogger(class SourceManager *sourceManager);

    /**
     * Method called when a new diagnostic message is emitted. It will log its content using EzLogger. Depending on
     * the configuration it may be printed to console and/or saved to a file.
     */
    void onDiag(const DiagnosticMessage &msg) override;

  private:
    /**
     * Logs the given note.
     */
    void logNote(LogMessage &msg, const DiagnosticNote &note);

    /**
     * Logs the name of the diagnostic message type.
     */
    void logType(LogMessage &msg, DiagnosticMessageType type);

    /**
     * Logs the given source reference, if it is != nullptr and valid.
     */
    void logSourceRef(LogMessage &msg, class SourceReference *sourceRef);

  private:
    SourceManager *m_sourceManager;   // Resolves source references into file/line text for log output.
    std::unique_ptr<Logger> m_logger; // Underlying EzLogger sink that receives each formatted diagnostic line.
};

#endif // EZCORE_DIAGNOSTIC_LOGGER_H
