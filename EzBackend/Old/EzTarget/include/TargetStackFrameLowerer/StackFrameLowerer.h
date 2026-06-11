#ifndef EZPACKER_STACKFRAMELOWERER_H
#define EZPACKER_STACKFRAMELOWERER_H

#include "EzTargetCommon.h"
#include "StackFrameLowererContext.h"

class StackFrameLowererPass : public IMirTransformPass
{
  public:
    StackFrameLowererPass(StackFrameLowererContext *ctx);

    bool run(TypedPoolLinkedList<class MirFunction> *funcList,
             TypedPoolLinkedList<class MirFunction>::Iterator it,
             class MirPassManager *passManager) override;

    MirPassIterationPlace getIterationPlace() const override;

  private:
    /**
     * Calculates the offsets of each frame object of the given function, returns true if succeeded.
     * @param func
     * @return
     */
    bool calculateStackFrameOffsets(MirFunction *func);

    /**
     * Inserts the prologue of the function and returns true if succeeded.
     * @return
     */
    bool insertPrologue();
    
    /**
     * Inserts the prologue of the function and returns true if succeeded.
     * @return
     */
    bool insertEpilogue();

    const char *getName() const override;
    
  private:
    int64_t m_stackFrameEndOffset;
    StackFrameLowererContext *m_ctx;
};

#endif // EZPACKER_STACKFRAMELOWERER_H
