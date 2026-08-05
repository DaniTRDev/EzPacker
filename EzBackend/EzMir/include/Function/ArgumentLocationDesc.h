#ifndef EZPACKER_ARGUMENTLOCATIONDESC_H
#define EZPACKER_ARGUMENTLOCATIONDESC_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"
#include "MirFunctionStackFrame.h"

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
    RegisterRef m_ref;
    size_t m_sizeBytes;
};

/**
 * Returns the stack object at which the argument is going to be placed.
 */
struct StackLoc
{
    size_t m_sizeBytes;
    StackFrameObject *m_object;
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
    RegisterRef m_reg;
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

    // The pointer to the data is either in a register OR sitting on the incoming stack slot area
    RegisterRef m_pointerStorage;
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
     * @param reg
     * @param sizeInBytes
     * @return
     */
    static ArgumentLocationDesc Reg(RegisterRef reg, size_t sizeInBytes);

    /**
     * Creates an indirect location with the given parameters.
     * @param byVal
     * @param copyOnReg
     * @param size
     * @param ptrStorage
     * @return
     */
    static ArgumentLocationDesc Indirect(bool byVal, bool copyOnReg, size_t size, RegisterRef ptrStorage);

    /**
     * Creates a split location with the given parameters.
     * @param regs
     * @return
     */
    static ArgumentLocationDesc Split(const std::vector<SplitPiece> &pieces);

    /**
     * Creates a stack location with the given parameters.
     */
    static ArgumentLocationDesc Stack(size_t sizeInBytes, StackFrameObject *object);

    /**
     * Returns the type of the location.
     * @return
     */
    ArgLocationType getType() const;

    /**
     * Returns the indirect location (IndirectLoc) of the argument. If internal type is not the one expected, an
     * exception is thrown.
     * @return
     */
    const IndirectLoc &getIndirect() const;

    /**
     * Returns the register location (RegisterLoc) of the argument. If internal type is not the one expected, an
     * exception is thrown.
     * @return
     */
    const RegLoc &getReg() const;

    /**
     * Returns the split location (SplitLoc) of the argument. If internal type is not the one expected, an exception is
     * thrown.
     * @return
     */
    const SplitLoc &getSplit() const;

    /**
     * Returns the stack location (StackLoc) of the argument. If internal type is not the one expected, an exception is
     * thrown.
     * @return
     */
    const StackLoc &getStack() const;

  private:
    /**
     * Creates an argument location with the given parameters. Constructor is made private so that the factory methods
     * are used.
     * @param type
     * @param storage
     */
    ArgumentLocationDesc(ArgLocationType type, StorageT storage);

  private:
    ArgLocationType m_type;
    StorageT m_storage;
};

#endif // EZPACKER_ARGUMENTLOCATIONDESC_H
