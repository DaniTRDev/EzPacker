#include "TargetStackFrameLowerer/StackFrameLowererContext.h"

StackFrameLowererContext::StackFrameLowererContext(MirEmitter *emitter, TargetDesc *desc) :
    m_emitter(emitter), m_desc(desc)
{
}

MirEmitter *StackFrameLowererContext::getEmitter() const { return m_emitter; }

TargetDesc *StackFrameLowererContext::getTargetDesc() const { return m_desc; }
