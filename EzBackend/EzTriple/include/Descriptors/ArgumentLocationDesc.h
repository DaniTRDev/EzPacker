#ifndef EZPACKER_ARGUMENTLOCATIONDESC_H
#define EZPACKER_ARGUMENTLOCATIONDESC_H

#include "EzTripleCommon.h"

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
    PhysicalRegId m_regId;
    size_t m_sizeBytes;
};

struct StackLoc
{
    int64_t m_frameOffset;
    size_t m_sizeBytes;
};

struct SplitLoc
{
    std::vector<RegLoc> m_parts;
};

struct IndirectLoc
{
    bool m_isByVal;
    size_t m_size;

    // The pointer to the data is either in a register OR sitting on the incoming stack slot area
    std::variant<PhysicalRegId, int64_t> m_pointerStorage;
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
     * @param regId
     * @param sizeInBytes
     * @return
     */
    static ArgumentLocationDesc Reg(PhysicalRegId regId, size_t sizeInBytes);

    /**
     * Creates an indirect location with the given parameters.
     * @param byVal
     * @param size
     * @param ptrStorage
     * @return
     */
    static ArgumentLocationDesc Indirect(bool byVal, size_t size, std::variant<PhysicalRegId, int64_t> ptrStorage);

    /**
     * Creates a split location with the given parameters.
     * @param regs
     * @return
     */
    static ArgumentLocationDesc Split(std::vector<RegLoc> regs);

    /**
     * Creates a stack location with the given parameters.
     * @param offset
     * @param sizeInBytes
     * @return
     */
    static ArgumentLocationDesc Stack(int64_t offset, size_t sizeInBytes);

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
