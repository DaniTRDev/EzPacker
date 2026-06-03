#ifndef EZPACKER_IDIAGNOSTICLISTENER_H
#define EZPACKER_IDIAGNOSTICLISTENER_H

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
     * @param msg
     */
    virtual void onDiag(const class DiagnosticMessage &msg) = 0;
};

#endif // EZPACKER_IDIAGNOSTICLISTENER_H
