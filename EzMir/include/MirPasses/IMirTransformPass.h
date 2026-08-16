#ifndef EZMIR_IMIR_TRANSFORM_PASS_H
#define EZMIR_IMIR_TRANSFORM_PASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

class IMirTransformPass : public MirPass
{
  public:
    virtual ~IMirTransformPass() = default;

    /**
     * Returns 'MirPassType::Transform' for this pass.
     */
    MirPassType getPassType() const override { return MirPassType::Transform; }
};

#endif // EZMIR_IMIR_TRANSFORM_PASS_H
