#ifndef EZPACKER_TARGETDESC_H
#define EZPACKER_TARGETDESC_H

#include "EzTripleCommon.h"

/**
 * Interface used to store target-dependent information (CPU-level).
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
     * Returns the type layout for this target.
     */
    virtual IMirTargetTypeLayout *getTypeLayout() = 0;

    /**
     * Returns the expansion registry used during expand action.
     */
    virtual class MirExpansionRuleRegistry *getExpansionRegistry() = 0;

    /**
     * Returns the frame lowerer for this target.
     */
    virtual class MirFrameLowerer *getFrameLowerer() = 0;

    /**
     * Returns the instruction selector required for this target.
     */
    virtual class MirInstructionSelector *getInstructionSelector() = 0;

    /**
     * Returns the legalizer needed for this target.
     */
    virtual class MirLegalizer *getLegalizer() = 0;

    /**
     * Returns the register allocator needed for this target.
     */
    virtual class MirRegisterAllocator *getRegisterAllocator() = 0;

    /**
     * Returns the displacement's type of a memory operand.
     */
    virtual MirType *getMemOperandDisplacementType() = 0;

    /**
     * Returns the nearest compatible type for the given type. If the type is already legal, it is returned as-is. If no
     * type can be used, nullptr will be returned.
     *
     * Examples 1: using an i1 (1-bit integer) is not possible in x64 arithmetic instructions, but might be allowed for
     * dev convenience, it must be promoted to the first legal type, which is i8 (8-bit).
     *
     * TODO: Use the table-driven approach the legalizer current has to enforce the legal type directly on the
     * definition of the legalization rule.
     * @param type
     * @return
     */
    virtual MirType *getNearestLegalType(MirType *type) = 0;

    /**
     * Returns the size in bytes of a standard stack slot (e.g., 8 for 64-bit targets, 4 for 32-bit).
     */
    virtual size_t getStackSlotSize() const = 0;

    /**
     * Initializes the target descriptor. This is the function that starts creating everything needed by the descriptor.
     */
    virtual void initialize() = 0;

    /**
     * Returns a list with the available calling conventions defined for this target.
     */
    virtual std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() = 0;

    /**
     * Returns a list with the available register banks for this target.
     */
    virtual std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() = 0;
};

#endif // EZPACKER_TARGETDESC_H
