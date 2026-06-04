#ifndef EZPACKER_DIAGNOSTICLOGGER_H
#define EZPACKER_DIAGNOSTICLOGGER_H

#include "EzCoreCommon.h"
#include "DiagnosticListener.h"
#include "DiagnosticCollector.h"

/**
 * Wrap between EzLogger and the diagnostic collector.
 */
class DiagnosticLogger : public DiagnosticListener
{
  public:
    /**
     * Creates the logger linked to the given source manager.
     * @param sourceManager
     */
    DiagnosticLogger(SourceManager *sourceManager);

    /**
     * Method called when a new diagnostic message is emitted. It will log its content using EzLogger. Depending on
     * the configuration it may be printed to console and/or saved to a file.
     * @param msg
     */
    void onDiag(const class DiagnosticMessage &msg) override;

  private:
    /**
     * Logs the given note.
     * @param msg
     * @param note
     */
    void logNote(LogMessage &msg, const DiagnosticNote &note);
    
    /**
     * Logs the name of the diagnostic message type.
     * @param type
     * @return
     */
    void logType(LogMessage &msg, DiagnosticMessageType type);

    /**
     * Logs the given source reference, if is != nullptr and valid.
     * @param msg
     * @param sourceRef
     */
    void logSourceRef(LogMessage &msg, const SourceReference &sourceRef);
    
  private:
    SourceManager *m_sourceManager;
    std::unique_ptr<Logger> m_logger;
};

#endif // EZPACKER_DIAGNOSTICLOGGER_H
