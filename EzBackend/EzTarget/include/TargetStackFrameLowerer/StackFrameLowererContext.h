#ifndef EZPACKER_STACKFRAMELOWERERCONTEXT_H
#define EZPACKER_STACKFRAMELOWERERCONTEXT_H

#include "EzCoreCommon.h"
#include "TargetDesc.h"

class StackFrameLowererContext
{
  public:
    StackFrameLowererContext(MirEmitter *emitter, TargetDesc *desc);

    MirEmitter *getEmitter() const;

    TargetDesc *getTargetDesc() const;

  private:
    MirEmitter *m_emitter;
    TargetDesc *m_desc;
};

#endif // EZPACKER_STACKFRAMELOWERERCONTEXT_H
