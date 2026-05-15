#ifndef EZPACKER_TARGETABILOWERERCONTEXT_H
#define EZPACKER_TARGETABILOWERERCONTEXT_H

#include "EzTargetCommon.h"
#include "TargetDesc.h"

class TargetAbiLowererContext
{
  public:
    /**
     * Constructor.
     * @param emitter
     * @param targetDesc
     */
    TargetAbiLowererContext(MirEmitter *emitter, TargetDesc *targetDesc);
    
    /**
     * Returns the emitter.
     * @return
     */
    MirEmitter *getEmitter() const;

    /**
     * Returns the target description.
     * @return
     */
    TargetDesc *getTargetDesc() const;

    /**
     * Returns the stack frame pool. Used to create function's stack frames.
     * @return
     */
    TypedPool *getStackFramePool();

    /**
     * Returns the stack frame object pool. Used to allocate stack objects within stack frames.
     * @return
     */
    TypedPool *getStackFrameObjectPool();

  private:
    MirEmitter *m_emitter;
    TargetDesc *m_targetDesc;

    TypedPool m_stackFramePool;
    TypedPool m_stackFrameObjectPool;
};

#endif // EZPACKER_TARGETABILOWERERCONTEXT_H
