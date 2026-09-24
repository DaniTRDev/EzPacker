#include "Function/MirFunctionStackFrame.h"
#include "Type/MirType.h"

/**
 * Takes ownership of an existing arena-allocated list of stack frame objects.
 */
MirFunctionStackFrame::MirFunctionStackFrame(std::pmr::vector<StackFrameObject *> stackFrameObjs) :
    m_stackFrameObjects(std::move(stackFrameObjs))
{
}

/**
 * Returns how many stack objects have been allocated in this frame so far.
 */
size_t MirFunctionStackFrame::getAllocatedObjectCount() const { return m_stackFrameObjects.size(); }

/**
 * Allocates a stack object for a source-level variable; the offset is assigned later by the
 * frame lowerer, so it starts at 0.
 */
StackFrameObject *MirFunctionStackFrame::createStaticStackObj(MirType *type)
{
    return create(0, type, StackFrameObjectSource::Variable);
}

/**
 * Allocates a stack object used as a register spill slot, initially with offset 0.
 */
StackFrameObject *MirFunctionStackFrame::createStackSpill(MirType *type)
{
    return create(0, type, StackFrameObjectSource::Spill);
}

/**
 * Allocates a stack object representing an incoming stack-passed parameter, initially with offset 0.
 */
StackFrameObject *MirFunctionStackFrame::createStackParam(MirType *type)
{
    return create(0, type, StackFrameObjectSource::Parameter);
}

/**
 * Allocates and registers a StackFrameObject, assigning it the next sequential ID equal to its
 * position in the object list.
 */
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

/**
 * Resolves a stack object by its ID, returning nullptr when the ID is out of range.
 */
StackFrameObject *MirFunctionStackFrame::getObjectFromId(MirId id)
{
    if (id < m_stackFrameObjects.size())
    {
        return m_stackFrameObjects[id];
    }
    return nullptr;
}

/**
 * Returns the full list of stack frame objects owned by this frame.
 */
const std::pmr::vector<StackFrameObject *> &MirFunctionStackFrame::getObjects() const { return m_stackFrameObjects; }
