#ifndef EZMIR_IMIR_TRANSFORM_PASS_H
#define EZMIR_IMIR_TRANSFORM_PASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

/**
 * Base abstract class for mutating MIR transformation passes (e.g. NonSsaToSsa, RegisterAllocator, FrameLowerer).
 */
class IMirTransformPass : public MirPass
{
  public:
    virtual ~IMirTransformPass() = default;

    /**
     * Returns MirPassType::Transform for all transformation passes.
     */
    MirPassType getPassType() const override { return MirPassType::Transform; }
};

#endif // EZMIR_IMIR_TRANSFORM_PASS_H
