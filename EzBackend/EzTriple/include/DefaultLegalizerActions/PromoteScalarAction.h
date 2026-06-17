#ifndef EZPACKER_PROMOTESCALARACTION_H
#define EZPACKER_PROMOTESCALARACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/LegalizeAction.h"

class PromoteScalarAction : public LegalizeAction
{
  public:
    /**
     * Creates the action linked to a context and a target.
     * @param ctx
     * @param target
     */
    PromoteScalarAction(MirBuilderContext *ctx, TargetDesc *target);

    /**
     * Returns "Promote"
     * @return
     */
    const char *getName() override;

    /**
     * Executes the given legalize action on the given instruction (pointed by the iterator received).
     * @param instrList
     * @param it
     * @return
     */
    LegalizeActionResult run(std::pmr::list<class MirInstruction *> &instrList,
                             std::pmr::list<class MirInstruction *>::iterator it) override;

  private:
    MirBuilderContext *m_ctx;
    TargetDesc *m_target;
};

#endif // EZPACKER_PROMOTESCALARACTION_H
