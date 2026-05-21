#ifndef EZPACKER_ABIDESC_H
#define EZPACKER_ABIDESC_H

#include "EzABICommon.h"
#include "ArgLocation.h"
#include "StackLayout.h"

class MirType; // Forward declaration

class ABIDesc
{
  public:
    /**
     * @brief Constructs an ABIDesc with default values.
     */
    ABIDesc();

    /**
     * @brief Get the location of the return value.
     * @returns The location of the return value.
     */
    const ArgLocation &getReturnValueLoc() const;

    /**
     * @brief Returns the location of the given argument.
     * @param id The argument index.
     * @param type The type of the argument.
     * @returns The location where the argument is passed.
     */
    virtual ArgLocation getArgLoc(size_t id, MirType *type) const = 0;

    /**
     * @brief Returns the name of the ABI.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * @brief Get the location of the stack frame pointer.
     * @returns The physical register ID of the frame pointer.
     */
    PhysicalRegId getStackFrameReg() const;

    /**
     * @brief Returns the stack register.
     * @returns The physical register ID of the stack pointer.
     */
    PhysicalRegId getStackReg() const;

    /**
     * @brief Returns the strict ABI alignment required for the given type.
     * @param type The type to check alignment for.
     * @returns The required alignment in bytes.
     */
    virtual size_t getAbiAlignment(MirType *type) const = 0;

    /**
     * @brief Get the size of registers in bits.
     * @returns The register size in bits.
     */
    size_t getRegSizeInBits() const;

    /**
     * @brief Returns the size of the stack offset.
     * @returns The stack offset size in bits.
     */
    size_t getStackOffsetSizeInBits();

    /**
     * @brief Get the stack layout for the function.
     * @returns The stack layout structure.
     */
    const StackLayout &getStackLayout() const;

    /**
     * @brief Set the location of the return value.
     * @param loc The new location for the return value.
     */
    void setReturnValueLoc(const ArgLocation &loc);

    /**
     * @brief Set the stack layout for the function.
     * @param layout The new stack layout.
     */
    void setStackLayout(const StackLayout &layout);

    /**
     * @brief Set the list of callee-saved registers.
     * @param regs The vector of physical register IDs.
     */
    void setCalleeSavedRegs(const std::vector<PhysicalRegId> &regs);

    /**
     * @brief Set the list of caller-saved registers.
     * @param regs The vector of physical register IDs.
     */
    void setCallerSavedRegs(const std::vector<PhysicalRegId> &regs);

    /**
     * @brief Set the size of registers in bits.
     * @param sizeInBits The register size in bits.
     */
    void setRegSizeInBits(size_t sizeInBits);

    /**
     * @brief Set the location of the stack frame pointer.
     * @param stackFrame The physical register ID for the frame pointer.
     */
    void setStackFrame(PhysicalRegId stackFrame);

    /**
     * @brief Set the stack offset size in bits.
     * @param sizeInBits The stack offset size in bits.
     */
    void setStackOffsetSize(size_t sizeInBits);

    /**
     * @brief Get the stack pointer register.
     * @param stackReg The physical register ID for the stack pointer.
     */
    void setStackReg(PhysicalRegId stackReg);

    /**
     * @brief Get the list of callee-saved registers.
     * @returns The vector of callee-saved physical register IDs.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedRegs() const;

    /**
     * @brief Get the list of caller-saved registers.
     * @returns The vector of caller-saved physical register IDs.
     */
    const std::vector<PhysicalRegId> &getCallerSavedRegs() const;

  private:
    ArgLocation m_returnValueLoc;
    PhysicalRegId m_stackFrame;
    PhysicalRegId m_stackReg;
    size_t m_regSizeInBits;
    size_t m_stackOffsetSizeInBits;
    StackLayout m_stackLayout;
    std::vector<PhysicalRegId> m_calleeSavedRegs;
    std::vector<PhysicalRegId> m_callerSavedRegs;
};

#endif // EZPACKER_ABIDESC_H