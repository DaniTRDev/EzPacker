#ifndef EZPACKER_CALLLOWERINGSTATE_H
#define EZPACKER_CALLLOWERINGSTATE_H

#include "EzMirCommon.h"
#include "CallingConvDesc.h"
#include "Builder/MirBuilderContext.h"
#include "Operand/MirOperands.h"

class CallLoweringState
{
  public:
    /**
     * Creates the state with the given calling convention and context.
     */
    CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx);

    /**
     * Attempts to allocate the next available register of the given class. Returns true if succeded and false if
     * no register is available.
     */
    bool allocate(RegisterRefClass refClass, RegisterRef &reg);

    /**
     * Returns the count of available registers of the given class type for the call.
     * @return
     */
    size_t getUsableRegCount(RegisterRefClass refClass) const;

    /**
     * Returns the count of used registers of the given class type for the call so far.
     * @return
     */
    size_t getUsedRegCount(RegisterRefClass refClass) const;

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
    CallingConvDesc *m_callingConv;
    int64_t m_currentStackOffset; // Current parameter stack frame offset (in bytes)

    // Record of registers allocated during this lowering state
    std::pmr::unordered_map<RegisterRefClass, std::pmr::vector<RegisterRef>> m_allocatedRegs;

    // Record of usable registers by this call lowering state machine.
    std::pmr::unordered_map<RegisterRefClass, std::pmr::vector<RegisterRef>> m_usableRegs;
};

#endif // EZPACKER_CALLLOWERINGSTATE_H
