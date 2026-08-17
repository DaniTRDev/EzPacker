#ifndef EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H
#define EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H

#include "Function/CallingConvDesc.h"
#include "Operand/MirRegisterReference.h"

class MirRegisterBank;
class MirRegisterClass;

/**
 * This calling convention is an empty shell, if used in proper env it will surely invoke UB or crash. This is just here
 * to feed MirFunction but its of no use in the tests of EzMir.
 */
class EzMirTestSuiteCallingConv : public CallingConvDesc
{
  public:
    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;

    bool canReturnInRegs(MirType *type) const override;
    bool isCalleeCleanup() const override { return false; } // Caller clean-up
    bool doesStackGrowsDownwards() const override { return true; }
    const char *getName() const override { return "EzMirTestSuiteCallingConv"; }

    MirRegisterRef getFramePointerReg() const override;
    MirRegisterRef getStackPointerReg() const override;

    size_t getStackAlignment() const override { return 16; }
    size_t getShadowSpaceSize() const override { return 0; }

    bool hasFramePointer(class MirFunction *func) const override;

    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override;
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override;

    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override;
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override;

  private:
    std::pmr::vector<MirRegisterRef> m_empty;
};

#endif // EZMIRTESTSUITE_EZ_MIR_TEST_SUITE_CALLING_CONV_H