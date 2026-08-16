#ifndef EZMIR_IMIR_ANALYSIS_PASS_H
#define EZMIR_IMIR_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

class IMirAnalysisPass : public MirPass
{
  public:
    virtual ~IMirAnalysisPass() = default;

    /**
     * Returns 'MirPassType::Analysis' for this pass.
     */
    MirPassType getPassType() const override { return MirPassType::Analysis; }
};

#endif // EZMIR_IMIR_ANALYSIS_PASS_H
