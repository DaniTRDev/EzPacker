#include "TargetAbiLowerer/TargetAbiLowererContext.h"

TargetAbiLowererContext::TargetAbiLowererContext(MirEmitter *emitter, TargetDesc *targetDesc) :
    m_emitter(emitter), m_targetDesc(targetDesc)
{
}

MirEmitter *TargetAbiLowererContext::getEmitter() const { return m_emitter; }

TargetDesc *TargetAbiLowererContext::getTargetDesc() const { return m_targetDesc; }

TypedPool *TargetAbiLowererContext::getStackFramePool() { return &m_stackFramePool; }

TypedPool *TargetAbiLowererContext::getStackFrameObjectPool() { return &m_stackFrameObjectPool; }
