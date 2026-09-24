#ifndef EZMIR_IMIR_ANALYSIS_PASS_H
#define EZMIR_IMIR_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPass.h"

/**
 * Base abstract class for read-only MIR analysis passes (e.g. CFG analysis, Liveness analysis, Dominator tree).
 */
class IMirAnalysisPass : public MirPass
{
  public:
    virtual ~IMirAnalysisPass() = default;

    /**
     * Returns MirPassType::Analysis for all analysis passes.
     */
    MirPassType getPassType() const override { return MirPassType::Analysis; }
};

#endif // EZMIR_IMIR_ANALYSIS_PASS_H
