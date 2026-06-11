#ifndef EZPACKER_IMIRTRANSFORMPASS_H
#define EZPACKER_IMIRTRANSFORMPASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

class IMirTransformPass : public MirPass
{
  public:
    virtual ~IMirTransformPass() = default;

    /**
     * Returns 'MirPassType::Transform' for this pass.
     * @return
     */
    MirPassType getPassType() const override { return MirPassType::Transform; }
};

#endif // EZPACKER_IMIRTRANSFORMPASS_H
