#include "Function/MirFunctionStackFrame.h"

MirFunctionStackFrame::MirFunctionStackFrame(std::pmr::vector<StackFrameObject *> stackFrameObjs) :
    m_stackFrameObjects(std::move(stackFrameObjs))
{
}

size_t MirFunctionStackFrame::getAllocatedObjectCount() const { return m_stackFrameObjects.size(); }

StackFrameObject *
MirFunctionStackFrame::create(int64_t offset, size_t align, size_t sizeInBytes, StackFrameObjectSource source)
{
    std::pmr::memory_resource *arena = m_stackFrameObjects.get_allocator().resource();
    std::pmr::polymorphic_allocator<StackFrameObject> objAlloc(arena);

    StackFrameObject *obj = objAlloc.allocate(1);
    obj->m_offset = offset;
    obj->m_align = align;
    obj->m_id = m_stackFrameObjects.size();
    obj->m_sizeInBytes = sizeInBytes;
    obj->m_source = source;

    m_stackFrameObjects.emplace_back(obj);
    return m_stackFrameObjects.back();
}

StackFrameObject *MirFunctionStackFrame::createLocalObject(size_t size, size_t align)
{
    return create(0, align, size, StackFrameObjectSource::Variable);
}

StackFrameObject *MirFunctionStackFrame::createSpill(size_t size, size_t align)
{
    return create(0, align, size, StackFrameObjectSource::Spill);
}

StackFrameObject *MirFunctionStackFrame::createParam(size_t size, size_t align, int64_t offset)
{
    return create(offset, align, size, StackFrameObjectSource::Parameter);
}

StackFrameObject *MirFunctionStackFrame::getObjectFromId(MirId id)
{
    if (id < m_stackFrameObjects.size())
    {
        return m_stackFrameObjects[id];
    }
    return nullptr;
}

const std::pmr::vector<StackFrameObject *> &MirFunctionStackFrame::getStackFrameObjects() const
{
    return m_stackFrameObjects;
}
