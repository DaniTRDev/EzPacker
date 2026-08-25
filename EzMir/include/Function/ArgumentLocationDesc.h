#ifndef EZMIR_ARGUMENT_LOCATION_DESC_H
#define EZMIR_ARGUMENT_LOCATION_DESC_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"

/**
 * Simple type used to abstract the register ID field as this may change in a future. THIS WILL COLLIDE
 * WITH MirId!
 */
using PhysicalRegId = size_t;

enum class ArgLocationType
{
    Invalid = 0,
    Indirect, // Used for objects, the compiler passes a pointer to the object in a register or stack location.
    Register, // The argument is passed into a specific physical register.
    Split,    // The argument is passed into a specific set of physical registers.
    Stack     // The argument is passed into a specific stack location.
};

struct RegLoc
{
    MirRegisterRef m_ref;
    size_t m_sizeBytes;
};

/**
 * Returns the stack object at which the argument is going to be placed.
 */
struct StackLoc
{
    size_t m_sizeBytes;
    class StackFrameObject *m_object;
};

/**
 * A structure might be returned in registers depending on the types of the fields.
 *
 * Example:
 * struct MyStruct
 * {
 *      int x;
 *      float y;
 * };
 *
 * FOR x ->
 * SplitLoc[0].m_regId = GPR
 * SplitLoc[0].m_sizeBytes = 4
 * SplitLoc[0]. m_offsetInParam = 0
 *
 * FOR y ->
 * SplitLoc[1].m_regId = FPR
 * SplitLoc[1].m_sizeBytes = 4
 * SplitLoc[1]. m_offsetInParam = 4
 */
struct SplitPiece
{
    MirRegisterRef m_reg;
    class MirType *m_type;
    size_t m_offsetInParam; // Byte offset from the start of the user's variable
};

struct SplitLoc
{
    std::vector<SplitPiece> m_parts;
};

struct IndirectLoc
{
    bool m_isByVal;
    bool m_copyOnReg; // Should the return ptr be copied into the return register?
    size_t m_size;

    // The pointer to the data is either in a register.
    MirRegisterRef m_pointerStorage;
};

/**
 * This class is used to determine in which place an argument will be lowered into. It may go to a register,
 * to a group of register or to stack.
 */
class ArgumentLocationDesc
{
  public:
    using StorageT = std::variant<RegLoc, StackLoc, SplitLoc, IndirectLoc>;

    /**
     * Creates a register location with the given parameters.
     */
    static ArgumentLocationDesc Reg(MirRegisterRef reg, size_t sizeInBytes);

    /**
     * Creates an indirect location with the given parameters.
     */
    static ArgumentLocationDesc Indirect(bool byVal, bool copyOnReg, size_t size, MirRegisterRef ptrStorage);

    /**
     * Creates a split location with the given parameters.
     */
    static ArgumentLocationDesc Split(const std::vector<SplitPiece> &pieces);

    /**
     * Creates a stack location with the given parameters.
     */
    static ArgumentLocationDesc Stack(size_t sizeInBytes, class StackFrameObject *object);

    /**
     * Returns the type of the location.
     */
    ArgLocationType getType() const;

    /**
     * Returns the indirect location (IndirectLoc) of the argument. If internal type is not the one expected, an
     * exception is thrown.
     */
    const IndirectLoc &getIndirect() const;

    /**
     * Returns the register location (RegisterLoc) of the argument. If internal type is not the one expected, an
     * exception is thrown.
     */
    const RegLoc &getReg() const;

    /**
     * Returns the split location (SplitLoc) of the argument. If internal type is not the one expected, an exception is
     * thrown.
     */
    const SplitLoc &getSplit() const;

    /**
     * Returns the stack location (StackLoc) of the argument. If internal type is not the one expected, an exception is
     * thrown.
     */
    const StackLoc &getStack() const;

  private:
    /**
     * Creates an argument location with the given parameters. Constructor is made private so that the factory methods
     * are used.
     */
    ArgumentLocationDesc(ArgLocationType type, StorageT storage);

  private:
    ArgLocationType m_type;
    StorageT m_storage;
};

#endif // EZMIR_ARGUMENT_LOCATION_DESC_H
