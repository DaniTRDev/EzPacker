#ifndef EZPACKER_TARGETABILOWERERPASS_H
#define EZPACKER_TARGETABILOWERERPASS_H

#include "EzTargetCommon.h"
#include "TargetAbiLowererContext.h"

class TargetAbiLowererPass : public IMirTransformPass
{
  public:
    /**
     * Constructs a TypeLegalizerPass with the given action and handler lists. The pass will use the action list to
     * determine what legalization actions to perform on illegal instructions, and will use the handler list to execute
     * any custom legalization logic for instructions that require it.
     * @param actionList
     * @param handlerList
     */
    TargetAbiLowererPass(TargetAbiLowererContext *ctx);

    /**
     * Runs the legalizer in the given function. It will change how arguments are used, how functions are called inside
     * the given function and how the function exits. Returns true if the function was modified, false otherwise.
     */
    bool run(TypedPoolLinkedList<class MirFunction> *funcList,
             TypedPoolLinkedList<class MirFunction>::Iterator it,
             class MirPassManager *passManager) override;

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

  private:
    /**
     * Lowers the parameters of a function into physical locations (regs or stack frame).
     * @param abi
     * @param emitter
     * @param func
     * @return
     */
    bool lowerParameters(ABIDesc *abi, MirEmitter *emitter, MirEmitterContext *emitterCtx, MirFunction *func);

    /**
     * Lowers the call site of a function.
     * @param block
     * @param it
     * @param abi
     * @param emitter
     * @param emitterCtx
     * @return
     */
    bool lowerCallSite(class MirBlock *block,
                       TypedPoolLinkedList<class MirInstruction>::Iterator it,
                       class ABIDesc *abi,
                       class MirEmitter *emitter,
                       class MirEmitterContext *emitterCtx);

    /**
     * Lowers the return site of a function.
     * @param block
     * @param it
     * @param abi
     * @param emitter
     * @param emitterCtx
     * @return
     */
    bool lowerReturnSite(class MirBlock *block,
                         TypedPoolLinkedList<class MirInstruction>::Iterator it,
                         class ABIDesc *abi,
                         class MirEmitter *emitter,
                         class MirEmitterContext *emitterCtx);

  private:
    TargetAbiLowererContext *m_ctx;
};

#endif // EZPACKER_TARGETABILOWERERPASS_H
