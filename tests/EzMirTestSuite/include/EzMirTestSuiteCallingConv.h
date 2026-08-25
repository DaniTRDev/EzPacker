#ifndef EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H
#define EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H

#include "Function/CallingConvDesc.h"
#include "Operand/MirRegisterReference.h"

class MirRegisterBank;
class MirRegisterClass;

/**
 * Mock Calling Convention implementation used exclusively for MIR test suite execution.
 * Provides stubbed and default behaviors for parameter locations, return locations,
 * stack direction, alignment, and register classification to allow MIR construction
 * and pass testing in an isolated environment.
 */
class EzMirTestSuiteCallingConv : public CallingConvDesc
{
  public:
    /**
     * Determines argument placement; returns default stack location for testing.
     */
    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Determines return value placement; returns default stack location for testing.
     */
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;

    /**
     * Indicates whether a given return type can be passed in registers (always false for mock).
     */
    bool canReturnInRegs(MirType *type) const override;

    /**
     * Indicates whether stack cleanup is performed by callee (caller cleanup assumed).
     */
    bool isCalleeCleanup() const override { return false; } // Caller clean-up

    /**
     * Defines whether stack grows downward towards lower addresses (true by default).
     */
    bool doesStackGrowsDownwards() const override { return true; }

    /**
     * Checks if a function requires an explicit frame pointer.
     */
    bool hasFramePointer(class MirFunction *func) const override;

    /**
     * Returns human-readable identifier of the test calling convention.
     */
    const char *getName() const override { return "EzMirTestSuiteCallingConv"; }

    /**
     * Returns the physical frame pointer register reference (empty mock register).
     */
    MirRegisterRef getFramePointerReg() const override;

    /**
     * Returns the physical stack pointer register reference (empty mock register).
     */
    MirRegisterRef getStackPointerReg() const override;

    /**
     * Returns the required stack alignment in bytes (standard 16-byte alignment).
     */
    size_t getStackAlignment() const override { return 16; }

    /**
     * Returns the shadow space allocated on the stack in bytes (0 for mock).
     */
    size_t getShadowSpaceSize() const override { return 0; }

    /**
     * Classifies a MIR type into ABI argument categories (no-op in test harness).
     */
    void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const {}

    /**
     * Returns all callee-saved registers for this calling convention.
     */
    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override;

    /**
     * Returns callee-saved registers for a specific register class.
     */
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override;

    /**
     * Returns all caller-saved (scratch) registers for this calling convention.
     */
    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override;

    /**
     * Returns caller-saved registers for a specific register class.
     */
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override;

  private:
    std::pmr::vector<MirRegisterRef> m_empty;
};

#endif // EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H