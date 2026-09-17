#ifndef EZTRIPLE_TARGET_DESC_H
#define EZTRIPLE_TARGET_DESC_H

#include "EzTripleCommon.h"
#include "Operand/MirRegisterReference.h"

/**
 * Interface used to store target-dependent information (CPU-level).
 *
 * Ex: TargetDesc = AMD64, TargetBinaryDesc = AMD64_Windows | AMD64_Linux.
 * The calling convention is also dependant on the target binary desc (AMD64_Windows_Windows | AMD64_Linux_SysV)
 */

class TargetDesc
{
  public:
    virtual ~TargetDesc() = default;

    /**
     * Returns the name of the target.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the frame lowerer for this target.
     */
    virtual class MirFrameLowerer *getFrameLowerer() = 0;

    /**
     * Returns the instruction selector required for this target.
     */
    virtual class MirInstructionSelector *getInstructionSelector() = 0;

    /**
     * Returns the addressing mode matcher required for this target, or nullptr if none.
     */
    virtual class MirAddressingModeMatcher *getAddressingModeMatcher() { return nullptr; }

    /**
     * Returns the general-purpose integer register class for this target, or nullptr if unspecified.
     */
    virtual class MirRegisterClass *getGprClass() { return nullptr; }

    /**
     * Returns the legalizer needed for this target.
     */
    virtual class MirLegalizer *getLegalizer() = 0;

    /**
     * Returns the LegalizerInfo containing table-driven legality definitions for this target.
     */
    virtual class LegalizerInfo *getLegalizerInfo() = 0;

    /**
     * Returns the register allocator needed for this target.
     */
    virtual class MirRegisterAllocator *getRegisterAllocator() = 0;

    /**
     * Returns the displacement's type of a memory operand.
     */
    virtual class MirType *getMemOperandDisplacementType() = 0;

    /**
     * Returns a reference to the target's instruction pointer.
     */
    virtual MirRegisterRef getInstructionPtrReg() const = 0;

    /**
     * Returns the size in bytes of a standard stack slot (e.g., 8 for 64-bit targets, 4 for 32-bit).
     */
    virtual size_t getStackSlotSize() const = 0;

    /**
     * Initializes the target descriptor. This is the function that starts creating everything needed by the descriptor.
     */
    virtual void initialize() = 0;

    /**
     * Returns the name of the libcall symbol pointed by the given libcall symbol Id.
     */
    virtual std::string_view getLibcallStr(uint8_t symId) = 0;

    /**
     * Returns a list with the available binary descriptors.
     */
    virtual std::pmr::vector<class TargetBinaryDesc *> getAvailableBinaryDescriptors() = 0;

    /**
     * Returns a list with the available calling conventions defined for this target.
     */
    virtual std::pmr::vector<class CallingConvDesc *> getAvailableCallingConventions() = 0;

    /**
     * Returns a list with the available register banks for this target.
     */
    virtual std::pmr::vector<class MirRegisterBank *> getAvailableRegisterBanks() = 0;
};

#endif // EZTRIPLE_TARGET_DESC_H
