#ifndef EZMIR_MIR_FUNCTION_STACK_FRAME_H
#define EZMIR_MIR_FUNCTION_STACK_FRAME_H

#include "EzMirCommon.h"

/**
 * Origin classification of an object residing on the function stack frame.
 */
enum class StackFrameObjectSource : uint8_t
{
    Invalid = 0,
    Parameter, // The object comes from an incoming stack argument.
    Variable,  // The object comes from a local stack variable.
    Spill      // The object comes from a register allocator spill slot.
};

/**
 * Single allocated slot on the function stack frame with byte offset, type, and source origin.
 */
struct StackFrameObject
{
    // Byte offset relative to frame pointer / stack pointer calculated during frame lowering
    int64_t m_offset{ 0 };
    class MirType *m_type;           // Type describing the size/alignment of the object.
    size_t m_id;                     // Sequential ID assigned from the object's position in the frame.
    StackFrameObjectSource m_source; // Why the object exists (parameter, local or spill).
};

/**
 * Stack frame layout manager tracking local variables, incoming stack parameters, and spill slots.
 */
class MirFunctionStackFrame
{
  public:
    /**
     * Constructs a stack frame instance initialized with an arena-managed object list.
     */
    MirFunctionStackFrame(std::pmr::vector<StackFrameObject *> objectList);

    /**
     * Returns the total count of stack frame objects allocated in this frame.
     */
    size_t getAllocatedObjectCount() const;

    /**
     * Allocates a local variable stack slot of the specified type.
     */
    StackFrameObject *createStaticStackObj(class MirType *type);

    /**
     * Allocates a spill slot of the specified type for register allocation.
     */
    StackFrameObject *createStackSpill(class MirType *type);

    /**
     * Allocates an incoming parameter stack slot of the specified type.
     */
    StackFrameObject *createStackParam(class MirType *type);

    /**
     * Creates a customized stack frame object with explicit offset, type, and source classifier.
     */
    StackFrameObject *create(int64_t offset, class MirType *type, StackFrameObjectSource source);

    /**
     * Retrieves a stack frame object by its numeric MirId. Returns nullptr if not found.
     */
    StackFrameObject *getObjectFromId(MirId id);

    /**
     * Returns the collection of all stack frame objects in this function frame.
     */
    const std::pmr::vector<StackFrameObject *> &getObjects() const;

  private:
    std::pmr::vector<StackFrameObject *> m_stackFrameObjects; // Allocated objects; index doubles as their ID.
};

#endif // EZMIR_MIR_FUNCTION_STACK_FRAME_H
