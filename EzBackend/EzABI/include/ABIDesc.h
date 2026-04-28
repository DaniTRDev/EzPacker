#ifndef EZPACKER_ABIDESC_H
#define EZPACKER_ABIDESC_H

#include "EzABICommon.h"
#include "ArgLocation.h"
#include "StackLayout.h"

enum AbiEndianness
{
    LittleEndian,
    BigEndian
};

class ABIDesc
{
  public:
    /**
     * @brief Constructs an ABIDesc with default values.
     */
    ABIDesc();

    /**
     * @brief Get the endianness of the ABI.
     * @return AbiEndianness The endianness of the ABI.
     */
    const AbiEndianness &getEndianness() const;

    /**
     * @brief Get the location of the return value.
     * @return const ArgLocation& The location of the return value.
     */
    const ArgLocation &getReturnValueLoc() const;

    /**
     * @brief Get the location of the stack frame pointer.
     * @return
     */
    PhysicalRegId getStackFrameReg() const;

    /**
     * Returns the stack register.
     * @return
     */
    PhysicalRegId getStackReg() const;

    /**
     * @brief Get the size of registers in bits.
     * @return
     */
    size_t getRegSizeInBits() const;

    /**
     * Returns the size of the stack offset.
     * @return size_t
     */
    size_t getStackOffsetSizeInBits();

    /**
     * @brief Get the stack layout for the function.
     * @return
     */
    const StackLayout &getStackLayout() const;

    /**
     * @brief Set the list of argument registers.
     * @param regs
     */
    void setArgRegs(const std::vector<PhysicalRegId> &regs);

    /**
     * @brief Get the endianness of the ABI.
     * @param endianness
     */
    void setEndianness(AbiEndianness endianness);

    /**
     * @brief Set the location of the return value.
     * @param loc
     */
    void setReturnValueLoc(const ArgLocation &loc);

    /**
     * @brief Set the stack layout for the function.
     * @param layout
     */
    void setStackLayout(const StackLayout &layout);

    /**
     * @brief Set the list of callee-saved registers.
     * @param regs
     */
    void setCalleeSavedRegs(const std::vector<PhysicalRegId> &regs);

    /**
     * @brief Set the list of caller-saved registers.
     * @param regs
     */
    void setCallerSavedRegs(const std::vector<PhysicalRegId> &regs);

    /**
     * @brief Set the size of registers in bits.
     * @param sizeInBits
     */
    void setRegSizeInBits(size_t sizeInBits);

    /**
     * @brief Set the location of the stack frame pointer.
     * @param stackFrame
     */
    void setStackFrame(PhysicalRegId stackFrame);

    /**
     * @brief Set the stack offset size in bits.
     * @param sizeInBits
     */
    void setStackOffsetSize(size_t sizeInBits);

    /**
     * @brief Get the stack pointer register.
     * @param stackReg
     */
    void setStackReg(PhysicalRegId stackReg);

    /**
     * @brief Get the list of argument registers.
     * @return
     */
    const std::vector<PhysicalRegId> &getArgRegs() const;

    /**
     * @brief Get the list of callee-saved registers.
     * @return
     */
    const std::vector<PhysicalRegId> &getCalleeSavedRegs() const;

    /**
     * @brief Get the list of calleer-saved registers.
     * @return
     */
    const std::vector<PhysicalRegId> &getCallerSavedRegs() const;

  private:
    AbiEndianness m_endianess;
    ArgLocation m_returnValueLoc;
    PhysicalRegId m_stackFrame;
    PhysicalRegId m_stackReg;
    size_t m_regSizeInBits;
    size_t m_stackOffsetSizeInBits;
    StackLayout m_stackLayout;
    std::vector<PhysicalRegId> m_argRegs;
    std::vector<PhysicalRegId> m_calleeSavedRegs;
    std::vector<PhysicalRegId> m_callerSavedRegs;
};

#endif // EZPACKER_ABIDESC_H
