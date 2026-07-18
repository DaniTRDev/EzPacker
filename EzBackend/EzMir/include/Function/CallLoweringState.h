#ifndef EZPACKER_CALLLOWERINGSTATE_H
#define EZPACKER_CALLLOWERINGSTATE_H

#include "EzMirCommon.h"
#include "ArgumentLocationDesc.h"

class CallLoweringState
{
  public:
    /**
     * Creates the state with the given usable registers (int and float).
     * @param usableGprs
     * @param usableFprs
     */
    CallLoweringState(const std::list<PhysicalRegId> &usableGprs,
                      const std::list<PhysicalRegId> &usableFprs);

    /**
     * Attempts to allocate the next available General Purpose Register.
     * @param outReg Set to the allocated register if successful.
     * @return true if a register was successfully allocated, false if none are left.
     */
    bool allocateGpr(PhysicalRegId &outReg);

    /**
     * Attempts to allocate the next available Floating Point Register.
     * @param outReg Set to the allocated register if successful.
     * @return true if a register was successfully allocated, false if none are left.
     */
    bool allocateFpr(PhysicalRegId &outReg);

    /**
     * Returns the count of available Floating Point Registers for the call.
     * @return
     */
    size_t getUsableFprCount() const;

    /**
     * Returns the count of available General Purpose Registers for the call.
     * @return
     */
    size_t getUsableGprCount() const;

    /**
     * Returns the count of used Floating Point Registers for the call so far.
     * @return
     */
    size_t getUsedFprCount() const;

    /**
     * Returns the count of used General Purpose Registers for the call so far.
     * @return
     */
    size_t getUsedGprCount() const;

    /**
     * Returns the current stack offset.
     * @return
     */
    int64_t getStackOffset() const;

    /**
     * @brief Dynamically allocates space on the incoming/outgoing parameter stack,
     * ensuring data properties line up with target alignment rules.
     * @param sizeBytes The data type footprint size.
     * @param alignmentBytes The strict data structure boundary layout mask.
     * @return The starting memory offset byte position relative to the stack frame anchor.
     */
    int64_t allocateStackSlot(size_t sizeBytes, size_t alignmentBytes);

  private:
    int64_t m_currentStackOffset; // Current parameter stack frame offset (in bytes)

    // Pool of usable registers.
    std::list<PhysicalRegId> m_usableFprs;
    std::list<PhysicalRegId> m_usableGprs;

    // Record of registers allocated during this lowering state
    std::vector<PhysicalRegId> m_allocatedFprs;
    std::vector<PhysicalRegId> m_allocatedGprs;
};

#endif // EZPACKER_CALLLOWERINGSTATE_H
