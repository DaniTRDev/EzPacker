#include "Function/MirFunctionStackFrame.h"

MirFunctionStackFrame::MirFunctionStackFrame(MirFunction *owner) : m_owner(owner), m_stackFrameObjects(nullptr) {}

MirFunctionStackFrame::MirFunctionStackFrame(MirFunction *owner, TypedPool *stackFrameObjectPool) :
    MirFunctionStackFrame(owner)
{
    m_stackFrameObjects = stackFrameObjectPool->createLinkedList<StackFrameObject>();
}

StackFrameObject *MirFunctionStackFrame::getObjectFromId(MirId id)
{
    for (auto it = m_stackFrameObjects->begin(); it != m_stackFrameObjects->end(); ++it)
    {
        StackFrameObject *obj = *it;
        if (obj->m_id == id)
        {
            return obj;
        }
    }
    return nullptr;
}

StackFrameObject *MirFunctionStackFrame::createLocalObject(size_t size, size_t align)
{
    StackFrameObject obj;
    obj.m_sizeInBytes = size;
    obj.m_align = align;
    obj.m_source = StackFrameObjectSource::Variable;

    return m_stackFrameObjects->m_owner->createAndAppendToListBack<StackFrameObject>(m_stackFrameObjects, obj);
}

StackFrameObject *MirFunctionStackFrame::createSpill(size_t size, size_t align)
{
    StackFrameObject obj;
    obj.m_sizeInBytes = size;
    obj.m_align = align;
    obj.m_source = StackFrameObjectSource::Spill;

    return m_stackFrameObjects->m_owner->createAndAppendToListBack<StackFrameObject>(m_stackFrameObjects, obj);
}

StackFrameObject *MirFunctionStackFrame::createParam(size_t size, size_t align, int64_t offset)
{
    StackFrameObject obj;
    obj.m_sizeInBytes = size;
    obj.m_align = align;
    obj.m_source = StackFrameObjectSource::Parameter;
    obj.m_offset = offset;

    return m_stackFrameObjects->m_owner->createAndAppendToListBack<StackFrameObject>(m_stackFrameObjects, obj);
}

void MirFunctionStackFrame::setOwner(struct MirFunction *owner) { m_owner = owner; }
