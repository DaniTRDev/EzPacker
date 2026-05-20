#ifndef EZPACKER_TYPELEGALIZERPASS_H
#define EZPACKER_TYPELEGALIZERPASS_H

#include "EzTargetCommon.h"
#include "LegalizerContext.h"
#include "TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h"
#include "TargetLegalizer/StandardLegalizers/ExpandTypeLegalizer.h"

class TypeLegalizerPass : public IMirTransformPass
{
  public:
    /**
     * Constructs a TypeLegalizerPass with the given action and handler lists. The pass will use the action list to
     * determine what legalization actions to perform on illegal instructions, and will use the handler list to execute
     * any custom legalization logic for instructions that require it.+
     * @param actionList
     * @param handlerList
     */
    TypeLegalizerPass(LegalizerContext *ctx);

    /**
     * Runs the legalizer pass on the given instruction list starting at given position. It will either
     * promote/expand/trunc the operands and results of instructions to legal types. Returns true if the instruction was
     * modified, false otherwise.
     */
    bool run(TypedPoolLinkedList<class MirInstruction> *instrList,
             TypedPoolLinkedList<class MirInstruction>::Iterator it,
             class MirPassManager *passManager) override;

    const char *getName() const override;
    
    /**
     * Returns MirPassIterationPlace::Instruction.
     */
    MirPassIterationPlace getIterationPlace() const override;

  private:
    LegalizerContext *m_ctx;
};

#endif // EZPACKER_TYPELEGALIZERPASS_H
