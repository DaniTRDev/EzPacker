#ifndef EZPACKER_TESTCALLINGCONVENTION_H
#define EZPACKER_TESTCALLINGCONVENTION_H

#include "Function/CallingConvDesc.h"

/**
 * @class TestCallingConvention
 * @brief A torture-test calling convention ("TestCallingConv") designed to stress-test
 * compiler lowering passes before moving to a production target architecture.
 *
 * Enforces strict alignment, intentional allocation discontinuities, and forced type slicing
 * to ensure robust behavior across return, call, argument, and frame-lowering phases.
 */
class TestCallingConvention : public CallingConvDesc
{
  public:
    /**
     * Creates the calling convention by populating callee-saved and caller-saved register vectors.
     * Sets up a small, highly restricted pool of volatile and preserved registers across
     * both GPR and FPR spaces to trigger early spilling and slicing constraints.
     */
    TestCallingConvention();

    /**
     * Returns the name identifying this testing ABI configuration ("TestCallingConv").
     */
    const char *getName() const override;

    /**
     * Maps function parameters to their physical locations. Alternates between registers and
     * stack slots based on current pool allocation parity to verify irregular packing routines.
     * Forcefully slices 64-bit scalars into multi-register Split locations to stress-test expansion logic.
     */
    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Determines return value storage targets. Maps values up to 64 bits to hardware registers,
     * and forces wider components into indirect Struct Return (SRET) address destinations.
     */
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Verifies if a given data type can be directly passed back inside hardware registers.
     * Returns true strictly for types that fit within 64 bits or less.
     */
    bool canReturnInRegs(MirType *type) const override;

    /**
     * Returns false to signify a caller-driven stack cleanup strategy (like cdecl or SysV),
     * validating the lowering engine's post-call adjustment placement math.
     */
    bool isCalleeCleanup() const override;

    /**
     * Enforces an aggressive 32-byte alignment boundary rule to test stack framework layout algorithms.
     */
    size_t getStackAlignment() const override;

    /**
     * Demands a custom, uneven 24-byte scratch/home allocation zone to check stack calculation bounds.
     */
    size_t getShadowSpaceSize() const override;

    /**
     * Returns the list of General Purpose Registers (GPRs) that must be preserved by the callee.
     * @return Reference to the callee-saved GPR vector.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedGPRegs() const override;

    /**
     * Returns the list of Floating Point Registers (FPRs) that must be preserved by the callee.
     * @return Reference to the callee-saved FPR vector.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedFPRegs() const override;

    /**
     * Returns the list of General Purpose Registers (GPRs) that must be preserved by the caller.
     * @return Reference to the caller-saved GPR vector.
     */
    const std::vector<PhysicalRegId> &getCallerSavedGPRegs() const override;

    /**
     * Returns the list of Floating Point Registers (FPRs) that must be preserved by the caller.
     * @return Reference to the caller-saved FPR vector.
     */
    const std::vector<PhysicalRegId> &getCallerSavedFPRegs() const override;

  private:
    std::vector<PhysicalRegId> m_gprCallerSaved; ///< Volatile General Purpose Registers
    std::vector<PhysicalRegId> m_fprCallerSaved; ///< Volatile Floating Point Registers

    std::vector<PhysicalRegId> m_gprCalleeSaved; ///< Non-volatile General Purpose Registers
    std::vector<PhysicalRegId> m_fprCalleeSaved; ///< Non-volatile Floating Point Registers
};

#endif // EZPACKER_TESTCALLINGCONVENTION_H