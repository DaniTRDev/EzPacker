#include "TargetRegisterAllocator/RegisterAllocatorContext.h"

RegisterAllocatorContext::RegisterAllocatorContext(MirEmitter *emitter, TargetDesc *targetDesc) :
    m_emitter(emitter), m_targetDesc(targetDesc)
{
}

MirEmitter *RegisterAllocatorContext::getEmitter() const { return m_emitter; }

TargetDesc *RegisterAllocatorContext::getTargetDesc() const { return m_targetDesc; }
