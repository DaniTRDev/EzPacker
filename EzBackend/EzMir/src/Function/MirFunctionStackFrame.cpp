#include "Function/MirFunctionStackFrame.h"

MirFunctionStackFrame::MirFunctionStackFrame(std::pmr::vector<StackFrameObject *> stackFrameObjs) :
    m_stackFrameObjects(std::move(stackFrameObjs))
{
}

size_t MirFunctionStackFrame::getAllocatedObjectCount() const { return m_stackFrameObjects.size(); }

StackFrameObject *MirFunctionStackFrame::createLocalStackObj(MirType *type)
{
    return create(0, type, StackFrameObjectSource::Variable);
}

StackFrameObject *MirFunctionStackFrame::createStackSpill(MirType *type)
{
    return create(0, type, StackFrameObjectSource::Spill);
}

StackFrameObject *MirFunctionStackFrame::createStackParam(MirType *type, int64_t offset)
{
    return create(offset, type, StackFrameObjectSource::Parameter);
}

StackFrameObject *MirFunctionStackFrame::create(int64_t offset, MirType *type, StackFrameObjectSource source)
{
    std::pmr::memory_resource *arena = m_stackFrameObjects.get_allocator().resource();
    std::pmr::polymorphic_allocator objAlloc(arena);

    StackFrameObject *obj = objAlloc.new_object<StackFrameObject>();
    obj->m_offset = offset;
    obj->m_type = type;
    obj->m_id = m_stackFrameObjects.size();
    obj->m_source = source;

    m_stackFrameObjects.emplace_back(obj);
    return m_stackFrameObjects.back();
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
