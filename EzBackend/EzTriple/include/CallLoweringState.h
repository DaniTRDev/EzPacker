#ifndef EZPACKER_CALLLOWERINGSTATE_H
#define EZPACKER_CALLLOWERINGSTATE_H

#include "EzTripleCommon.h"
#include "Descriptors/ABIDesc.h"

class CallLoweringState
{
  public:
    /**
     * Creates the state with the given ABI.
     * @param abiDesc
     */
    CallLoweringState(ABIDesc *abi);
    
    /**
     * @brief Grants read-only access to the underlying platform ABI description details.
     */
    ABIDesc *getABI() const;
    
    /**
     * Returns the count of available Floating Point Registers for the call.
     * @return
     */
    size_t getFprCount() const;
    
    /**
     * Returns the count of available General Purpose Registers for the call.
     * @return
     */
    size_t getGprCount() const;
    
    /**
     * Returns the current stack offset.
     * @return
     */
    int64_t getStackOffset() const;
    
    /**
     * Consumes a FPR.
     * @param count
     */
    void consumeFprs(size_t count);
    
    /**
     * Consumes a GPR.
     * @param count
     */
    void consumeGprs(size_t count);

    /**
     * @brief Dynamically allocates space on the incoming/outgoing parameter stack,
     * ensuring data properties line up perfectly with target alignment rules.
     * @param sizeBytes The data type footprint size.
     * @param alignmentBytes The strict data structure boundary layout mask.
     * @return The starting memory offset byte position relative to the stack frame anchor.
     */
    int64_t allocateStackSlot(size_t sizeBytes, size_t alignmentBytes);

  private:
    ABIDesc *m_abi;
    size_t m_allocatedGprs;       // Ticked-off standard integer/pointer registers
    size_t m_allocatedFprs;       // Ticked-off vector/floating-point registers
    int64_t m_currentStackOffset; // Running parameter stack cursor allocation metric (in bytes)
};

#endif // EZPACKER_CALLLOWERINGSTATE_H
