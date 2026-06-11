#ifndef EZPACKER_REGISTERALLOCATORCONTEXT_H
#define EZPACKER_REGISTERALLOCATORCONTEXT_H

#include "EzTargetCommon.h"
#include "TargetDesc.h"

class RegisterAllocatorContext
{
  public:
    /**
     * Creates the register allocator.
     * @param emitter
     * @param targetDesc
     */
    RegisterAllocatorContext(MirEmitter *emitter, TargetDesc *targetDesc);

    /**
     * Returns the emitter.
     * @return
     */
    MirEmitter *getEmitter() const;

    /**
     * Returns the target desc.
     * @return
     */
    TargetDesc *getTargetDesc() const;

  private:
    MirEmitter *m_emitter;
    TargetDesc *m_targetDesc;
};

#endif // EZPACKER_REGISTERALLOCATORCONTEXT_H
