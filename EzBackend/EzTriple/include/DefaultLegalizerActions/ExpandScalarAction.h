#ifndef EZPACKER_EXPANDSCALARACTION_H
#define EZPACKER_EXPANDSCALARACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/LegalizeAction.h"
#include "ExpansionRecipe/ExpansionRecipe.h"

/**
 * This action will expand an unsupported bigger type into smaller supported types. This expansion fully depends on the
 * architecture and the operations. Linear operations (addition, substraction) can be expanded into a low+high set of
 * operations with smaller types and only the carry bit needs to be taken care of.
 *
 * For non-linear operations like multiplication or division, this is a much more complex process that needs to perform
 * the operation straight without recurring to HW's instructions at all.
 */
class ExpandScalarAction : public LegalizeAction
{
  public:
    /**
     * Creates the action linked to a context and a target.
     * @param ctx
     * @param target
     */
    ExpandScalarAction(MirBuilderContext *ctx, TargetDesc *target);

    /**
     * Returns "ExpandScalarAction"
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

#endif // EZPACKER_EXPANDSCALARACTION_H
