#ifndef EZMIR_MIR_FUNCTION_STACK_FRAME_H
#define EZMIR_MIR_FUNCTION_STACK_FRAME_H

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
    // Filled by the prologue/epilogue pass (frame lowerer).
    int64_t m_offset{ 0 };
    class MirType *m_type;
    size_t m_id;
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
     * Creates a local object in the function's stack frame.
     * @param type
     * @return
     */
    StackFrameObject *createStaticStackObj(class MirType *type);

    /**
     * Creates an object resulting of a spill in the function's stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *createStackSpill(class MirType *type);

    /**
     * Creates a parameter in the function stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *createStackParam(class MirType *type);

    /**
     * Creates a specific stack frame object with the given parameters
     */
    StackFrameObject *create(int64_t offset, class MirType *type, StackFrameObjectSource source);

    /**
     * Returns a stack frame object out of an ID, if it was not found, nullptr is returned.
     */
    StackFrameObject *getObjectFromId(MirId id);

    /**
     * Returns the list of stack frame objects.
     */
    const std::pmr::vector<StackFrameObject *> &getObjects() const;

  private:
    std::pmr::vector<StackFrameObject *> m_stackFrameObjects;
};

#endif // EZMIR_MIR_FUNCTION_STACK_FRAME_H
