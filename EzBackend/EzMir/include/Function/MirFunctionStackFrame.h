#ifndef EZPACKER_MIRFUNCTIONSTACKFRAME_H
#define EZPACKER_MIRFUNCTIONSTACKFRAME_H

#include "EzMirCommon.h"

enum class StackFrameObjectSource : uint8_t
{
    Parameter, // The object comes from a parameter.
    Variable,  // The object comes from a variable.
    Spill      // The object comes from a spill.
};

/**
 * An object that lives the the stack frame of a function.
 */
struct StackFrameObject
{
    // Filled by the prologue/epilogue pass. Or if it's an ABI-enforced offset (like for parameters).
    int64_t m_offset{ 0 };
    size_t m_align{ 0 };
    size_t m_id{ 0 };
    size_t m_sizeInBytes{ 0 };
    StackFrameObjectSource m_source;
};

/**
 * Class used to describe a function's frame.
 */
class MirFunctionStackFrame
{
  public:
    /**
     * Default constructor to allow the first layer of MIR not to be concerned about creating the stack frame.
     */
    MirFunctionStackFrame(class MirFunction *owner);

    /**
     * Creates the stack frame for a function. Will use the pool to allocate a self-contained linked list of stack
     * frame objects.
     * @param owner
     * @param stackFrameObjectPool
     */
    MirFunctionStackFrame(class MirFunction *owner, TypedPool *stackFrameObjectPool);

    /**
     * Returns the allocated object count.
     * @return
     */
    size_t getAllocatedObjectCount() const;

    /**
     * Creates an abstract object in the function stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *createLocalObject(size_t size, size_t align);

    /**
     * Creates an abstract object in the function stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *createSpill(size_t size, size_t align);

    /**
     * Creates a parameter at the given offset. This is the only object whose offset is known at creation-time as this
     * is directly dictated by ABI.
     * @param size
     * @param align
     * @param offset
     * @return
     */
    StackFrameObject *createParam(size_t size, size_t align, int64_t offset);

    /**
     * Returns a stack frame object out of an ID, if it was not found, nullptr is returned.
     * @param id
     * @return
     */
    StackFrameObject *getObjectFromId(MirId id);

    /**
     * Returns the list of stack frame objects.
     * @return
     */
    TypedPoolLinkedList<StackFrameObject> *getStackFrameObjects() const;

    /**
     * Sets the owner of this stack frame.
     * @param m_owner
     */
    void setOwner(class MirFunction *m_owner);

  private:
    class MirFunction *m_owner;
    TypedPoolLinkedList<StackFrameObject> *m_stackFrameObjects;
};

#endif // EZPACKER_MIRFUNCTIONSTACKFRAME_H
