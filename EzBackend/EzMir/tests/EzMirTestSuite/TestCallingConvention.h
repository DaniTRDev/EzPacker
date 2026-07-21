#ifndef EZPACKER_TESTCALLINGCONVENTION_H
#define EZPACKER_TESTCALLINGCONVENTION_H

#include "Function/CallingConvDesc.h"

/**
 * @class TestCallingConvention
 * @brief A synthetic "torture-test" calling convention ("TestCallingConvention") designed to stress-test
 * compiler ABI lowering passes (Calls, Returns, SRET, Spills, Splits) using strict, constrained rules.
 *
 * Configured register layout:
 *   - Caller-Saved GPRs (Volatile):   { 1, 2 }
 *   - Callee-Saved GPRs (Preserved):  { 3 }
 *   - Caller-Saved FPRs (Volatile):   { 4 }
 *   - Callee-Saved FPRs (Preserved):  { 5 }
 *
 * Frame & Alignment constraints:
 *   - Mandatory pre-call stack alignment: 32 bytes
 *   - Shadow / Home space size:          24 bytes
 *   - Cleanup strategy:                  Caller cleanup (`isCalleeCleanup() == false`)
 */
class TestCallingConvention : public CallingConvDesc
{
  public:
    /**
     * Initializes the calling convention register pools. Sets up deliberately small,
     * restricted pools across GPRs {1, 2, 3} and FPRs {4, 5} to trigger early register
     * exhaustion, stack spills, and register clobbering.
     */
    TestCallingConvention();

    /**
     * Returns the identifying name of this testing ABI ("TestCallingConvention").
     * @return C-style string name.
     */
    const char *getName() const override;

    /**
     * Determines parameter placement according to the following evaluation sequence:
     *
     *   1. Size-Based Special Rules:
     *      - 8 bytes (64-bit values): Forced into a 2-part Split location (low 32-bit @ offset 0,
     *        high 32-bit @ offset 4) if 2 GPRs are available.
     *      - 4 bytes (32-bit values): Direct single-GPR register placement if a GPR is available.
     *      - 32 bytes (256-bit values): Forced into an Indirect location (`byVal=true`, `copyOnReg=false`).
     *        Allocates a GPR for the storage pointer if available, otherwise spills to a stack slot.
     *
     *   2. Register Parity Interleaving Rule (Torture Rule):
     *      - If the total allocated register count (`usedGPRs + usedFPRs`) is currently EVEN,
     *        the parameter is forcibly routed to a stack slot regardless of register availability.
     *
     *   3. Standard Register Fallback:
     *      - FloatingPoint types: Allocated to the next available FPR.
     *      - Integer/Pointer types: Allocated to the next available GPR.
     *
     *   4. Spill Fallback:
     *      - If no registers remain, allocates an aligned stack slot frame offset via `callState`.
     *
     * @param type The MIR data type of the argument.
     * @param callState The active call lowering state tracking register allocations and stack offsets.
     * @return `ArgumentLocationDesc` describing the parameter's destination.
     */
    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Determines return value location based on size and type kind:
     *   - Types > 64 bits (`!canReturnInRegs`): Lowered as an Indirect Struct Return (SRET) pointer
     *     (`byVal=true`, `copyOnReg=true`) stored in GPR 1.
     *   - FloatingPoint types ($\le$ 64 bits): Returned in volatile FPR 4.
     *   - All other types ($\le$ 64 bits): Returned in volatile GPR 1.
     *
     * @param type The MIR data type of the return value.
     * @param callState The active call lowering state context.
     * @return `ArgumentLocationDesc` describing where the callee places or writes the return value.
     */
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Dictates whether a type can be returned directly in registers.
     * @param type The MIR data type evaluated.
     * @return `true` if type footprint $\le$ 64 bits; `false` otherwise (forcing SRET lowering).
     */
    bool canReturnInRegs(MirType *type) const override;

    /**
     * Identifies stack cleanup responsibility.
     * @return `false` indicating caller-side stack cleanup (e.g., CDECL / System V).
     */
    bool isCalleeCleanup() const override;

    /**
     * Returns the mandatory pre-call stack boundary alignment requirement.
     * @return Always returns 32 bytes.
     */
    size_t getStackAlignment() const override;

    /**
     * Returns the required caller-allocated shadow space footprint preceding stack arguments.
     * @return Always returns 24 bytes.
     */
    size_t getShadowSpaceSize() const override;

    /**
     * Returns the vector of callee-saved (non-volatile) General Purpose Registers.
     * @return Const reference to vector containing GPR 3.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedGPRegs() const override;

    /**
     * Returns the vector of callee-saved (non-volatile) Floating Point Registers.
     * @return Const reference to vector containing FPR 5.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedFPRegs() const override;

    /**
     * Returns the vector of caller-saved (volatile) General Purpose Registers.
     * @return Const reference to vector containing GPRs {1, 2}.
     */
    const std::vector<PhysicalRegId> &getCallerSavedGPRegs() const override;

    /**
     * Returns the vector of caller-saved (volatile) Floating Point Registers.
     * @return Const reference to vector containing FPR 4.
     */
    const std::vector<PhysicalRegId> &getCallerSavedFPRegs() const override;

  private:
    std::vector<PhysicalRegId> m_gprCallerSaved; ///< Volatile GPRs: {1, 2}
    std::vector<PhysicalRegId> m_fprCallerSaved; ///< Volatile FPRs: {4}

    std::vector<PhysicalRegId> m_gprCalleeSaved; ///< Preserved GPRs: {3}
    std::vector<PhysicalRegId> m_fprCalleeSaved; ///< Preserved FPRs: {5}
};

#endif // EZPACKER_TESTCALLINGCONVENTION_H