#ifndef EZPACKER_PROMOTESCALARACTION_H
#define EZPACKER_PROMOTESCALARACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/LegalizeAction.h"

/**
 * This action will PROMOTE  a type. Promotion means that a smaller non-supported type gets promoted into a bigger type
 * that is actually supported by the target architecture.
 *
 * This is done by inserting ZEXT/SEXT/FPEXT/TRUNC instructions and modifying the operands of the affected instructions.
 */
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
     * Returns "PromoteScalarAction"
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
    std::map<size_t, MirRegister *> m_promotionMap; // Map used to store promoted registers.
};

#endif // EZPACKER_PROMOTESCALARACTION_H
