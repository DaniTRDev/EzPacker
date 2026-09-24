#include "EzMirTestSuiteCallingConv.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunction.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Type/MirType.h"

// Returns an empty/null register reference as mock frame pointer.
MirRegisterRef EzMirTestSuiteCallingConv::getFramePointerReg() const { return MirRegisterRef(); }

// Returns an empty/null register reference as mock stack pointer.
MirRegisterRef EzMirTestSuiteCallingConv::getStackPointerReg() const { return MirRegisterRef(); }

// Frame pointer is not required by default in the test calling convention.
bool EzMirTestSuiteCallingConv::hasFramePointer(MirFunction *func) const { return false; }

// Test calling convention defaults all return values to stack slots rather than physical registers.
bool EzMirTestSuiteCallingConv::canReturnInRegs(MirType *type) const { return false; }

// Directs all function arguments to stack offset 0 for test stub purposes.
ArgumentLocationDesc EzMirTestSuiteCallingConv::getArgLoc(MirType *type, CallLoweringState *callState)
{
    return ArgumentLocationDesc::Stack(0, nullptr);
}

// Directs all function return values to stack offset 0 for test stub purposes.
ArgumentLocationDesc EzMirTestSuiteCallingConv::getReturnLoc(MirType *type, CallLoweringState *)
{
    return ArgumentLocationDesc::Stack(0, nullptr);
}

// Returns empty list of callee-saved registers for test mock.
const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getAllCalleeSavedRegs() { return m_empty; }

// Returns empty list of callee-saved registers for the given register class.
const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getCalleeSavedRegs(MirRegisterClass *_class)
{
    return m_empty;
}

// Returns empty list of caller-saved (scratch) registers for test mock.
const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getAllCallerSavedRegs() { return m_empty; }

// Returns empty list of caller-saved registers for the given register class.
const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getCallerSavedRegs(MirRegisterClass *_class)
{
    return m_empty;
}