#ifndef EZCORE_IDIAGNOSTIC_LISTENER_H
#define EZCORE_IDIAGNOSTIC_LISTENER_H

#include "EzCoreCommon.h"

/**
 * Interface used to listen to diagnostic messages emitted into a diagnostic collector.
 */
class DiagnosticListener
{
  public:
    virtual ~DiagnosticListener() = default;

    /**
     * Method called when a new diagnostic message is emitted. It will be called by the collector automatically on
     * attached diagnostic subscribers.
     */
    virtual void onDiag(const class DiagnosticMessage &msg) = 0;
};

#endif // EZCORE_IDIAGNOSTIC_LISTENER_H
