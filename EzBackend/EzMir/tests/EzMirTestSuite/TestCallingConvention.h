#ifndef EZPACKER_TESTCALLINGCONVENTION_H
#define EZPACKER_TESTCALLINGCONVENTION_H

#include "Function/CallingConvDesc.h"

/**
 * @class TestCallingConvention
 * @brief An adversarial, torture-test calling convention ("ChaosConv") designed to stress-test
 * compiler lowering passes before moving to a production target architecture.
 *
 * Enforces strict alignment, intentional allocation discontinuities, and forced type slicing
 * to ensure robust behavior across return, call, argument, and frame-lowering phases.
 */
class TestCallingConvention : public CallingConvDesc
{
  public:
    /**
     * Creates the calling convention by populating callee saved registers and caller saved registers.
     * Sets up a small pool of volatile and preserved registers to force quick spills.
     */
    TestCallingConvention();

    /**
     * Returns the name identifying this testing ABI configuration ("TestCallingConvention").
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
     * Gets the collection of target platform registers that must be preserved across frame calls.
     */
    const std::vector<PhysicalRegId> &getCalleeSavedRegs() const override;

    /**
     * Gets the collection of target platform registers considered volatile across call bounds.
     */
    const std::vector<PhysicalRegId> &getCallerSavedRegs() const override;

  private:
    std::vector<PhysicalRegId> m_callerSaved;
    std::vector<PhysicalRegId> m_calleeSaved;
};

#endif // EZPACKER_TESTCALLINGCONVENTION_H