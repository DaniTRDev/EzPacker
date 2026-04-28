#ifndef EZPACKER_IMIRTRANSFORMPASS_H
#define EZPACKER_IMIRTRANSFORMPASS_H

#include "EzMirCommon.h"
#include "IMirPass.h"

class IMirTransformPass : public IMirPass
{
  public:
    virtual ~IMirTransformPass() = default;
    
    /**
     * Returns 'MirPassType::Analysis' for this pass.
     * @return
     */
    MirPassType getPassType() const override { return MirPassType::Transform; }
};

#endif // EZPACKER_IMIRTRANSFORMPASS_H
