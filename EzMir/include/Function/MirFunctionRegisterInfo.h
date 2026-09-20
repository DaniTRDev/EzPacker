#ifndef EZMIR_MIR_FUNCTION_REGISTER_INFO_H
#define EZMIR_MIR_FUNCTION_REGISTER_INFO_H

#include "EzMirCommon.h"

/**
 * One read of a virtual register: the instruction performing the read and the operand position.
 */
struct MirVRegUse
{
    class MirInstruction *m_userInst{ nullptr }; // Instruction that reads the register.
    size_t m_operandIndex{ 0 };                  // Index of the operand within that instruction.
};

/**
 * Per-register SSA bookkeeping: its single defining instruction and all use sites.
 */
struct MirVRegData
{
    class MirInstruction *m_defInst{ nullptr }; // Instruction that defines the register, or nullptr.
    std::pmr::vector<MirVRegUse> m_uses;        // All recorded reads of the register.

    /**
     * Allocates the use list from the given arena.
     */
    explicit MirVRegData(std::pmr::memory_resource *alloc) : m_uses(alloc) {}
};

/**
 * This class is a tracker that will watch over defs and uses of register across a MirFunction. It's updated on each
 * instruction addition, erasure or modification by MirInstructionBuilder.
 *
 * This supposes SSA form of the IR. (Single DEF / multiple uses perregister)
 */
class MirFunctionRegisterInfo
{
  public:
    /**
     * Creates the register tracker backed by the supplied arena.
     */
    explicit MirFunctionRegisterInfo(std::pmr::memory_resource *alloc);

    /**
     * Returns true if the given register has exactly 1 use.
     */
    bool hasOneUse(MirId regId) const;

    /**
     * Returns the definition place of the given register ID.
     */
    class MirInstruction *getDef(MirId regId) const;

    /**
     * Returns the use count of the given register ID.
     */
    size_t getUseCount(MirId regId) const;

    /**
     * Clears the definition of the given register.
     */
    void clearDef(MirId regId);

    /**
     * Records a definition of the given register by the given instruction.
     */
    void recordDef(MirId regId, class MirInstruction *inst);

    /**
     * Records an use of the given register by the given instruction.
     */
    void recordUse(MirId regId, class MirInstruction *inst, size_t opIndex);

    /**
     * Removed the use of the given register by the given instruction.
     */
    void removeUse(MirId regId, const class MirInstruction *inst);

    /**
     * Resets the internal map and clears it.
     */
    void reset();

    /**
     * Returns a pointer to the use list for a given register ID, or nullptr when the register is
     * unknown. The pointer is invalidated only by mutation of this tracker.
     */
    const std::pmr::vector<MirVRegUse> *getUses(MirId regId) const;

  private:
    std::pmr::unordered_map<MirId, MirVRegData> m_vregs; // Register ID -> SSA def/use information.
};

#endif // EZMIR_MIR_FUNCTION_REGISTER_INFO_H