#ifndef EZPACKER_IMIRANALYSISPASS_H
#define EZPACKER_IMIRANALYSISPASS_H

#include "EzMirCommon.h"
#include "IMirPass.h"

class IMirAnalysisPass : public IMirPass
{
  public:
    virtual ~IMirAnalysisPass() = default;
    
    /**
     * Returns 'MirPassType::Analysis' for this pass.
     * @return
     */
    MirPassType getPassType() const override { return MirPassType::Analysis; }
};

#endif // EZPACKER_IMIRANALYSISPASS_H
