#ifndef EZPACKER_IMIRPASS_H
#define EZPACKER_IMIRPASS_H

#include "EzMirCommon.h"

enum class MirPassType : uint8_t
{
    Analysis, // Only reads the MIR.
    Transform // Can apply changes to the MIR.
};

/**
 * Interface used by passes that iterate over the MIR.
 */
class IMirPass
{
  public:
    virtual ~IMirPass() = default;

    /**
     * Runs the pass on the given MIR func.
     */
    virtual bool run(class MirFunction *func, class MirPassManager *passManager) = 0;

    /**
     * Returns the pass type.
     * @return
     */
    virtual MirPassType getPassType() const = 0;
};

#endif // EZPACKER_IMIRPASS_H
