#ifndef EZPACKER_LEGALIZERETURNACTION_H
#define EZPACKER_LEGALIZERETURNACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/LegalizeAction.h"

/**
 * This action will split a single-operand return into a PUSH_RET and a no-operand RET.
 *
 * EXAMPLE -> RET i64 %ret1
 *
 * RESULT ->
 * PUSH_RET i64 %ret1
 * RET
 */
class LegalizeReturnAction : public LegalizeAction
{
  public:
    /**
     * Creates the action linked to a context.
     * @param ctx
     */
    LegalizeReturnAction(MirBuilderContext *ctx);

    /**
     * Returns "LegalizeReturnAction"
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
};

#endif // EZPACKER_LEGALIZERETURNACTION_H
