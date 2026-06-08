#ifndef EZPACKER_IMIRANALYSISPASS_H
#define EZPACKER_IMIRANALYSISPASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

class IMirAnalysisPass : public MirPass
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
