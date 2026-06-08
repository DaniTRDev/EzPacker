#ifndef EZPACKER_MIRFUNCTIONSTACKFRAME_H
#define EZPACKER_MIRFUNCTIONSTACKFRAME_H

#include "EzMirCommon.h"

enum class StackFrameObjectSource : uint8_t
{
    Invalid = 0,
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
     * Creates the stack frame of the function with the given object list. This list should be backend by an
     * arena somewhere and the arena must keep it alive until it's not needed at all.
     * @param objectList
     */
    MirFunctionStackFrame(std::pmr::vector<StackFrameObject *> objectList);

    /**
     * Returns the allocated object count.
     * @return
     */
    size_t getAllocatedObjectCount() const;

    /**
     * Creates a specific stack frame object with the given parameters
     * @param offset
     * @param align
     * @param sizeInBytes
     * @param source
     * @return
     */
    StackFrameObject *create(int64_t offset, size_t align, size_t sizeInBytes, StackFrameObjectSource source);

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
    const std::pmr::vector<StackFrameObject *> &getStackFrameObjects() const;

  private:
    std::pmr::vector<StackFrameObject *> m_stackFrameObjects;
};

#endif // EZPACKER_MIRFUNCTIONSTACKFRAME_H
