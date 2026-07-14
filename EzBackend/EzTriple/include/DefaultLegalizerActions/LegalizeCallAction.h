#ifndef EZPACKER_LEGALIZECALLACTION_H
#define EZPACKER_LEGALIZECALLACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/LegalizeAction.h"

/**
 * This action will pop function calling parameters and will push new instructions right before the call. This is done
 * so other legalization actions can affect parameters.
 *
 * EXAMPLE -> call %dest, i64 %arg1, i64 45
 *
 * RESULT ->
 * PUSH_ARG i64 %arg1
 * PUSH_ARG i64 45
 * call %dest
 */
class LegalizeCallAction : public LegalizeAction
{
  public:
    /**
     * Creates the action linked to a context and a target.
     * @param ctx
     * @param target
     */
    LegalizeCallAction(MirBuilderContext *ctx);

    /**
     * Returns "LegalizeCallAction"
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

#endif // EZPACKER_LEGALIZECALLACTION_H
