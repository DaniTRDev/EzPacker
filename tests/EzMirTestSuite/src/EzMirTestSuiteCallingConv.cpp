#include "EzMirTestSuiteCallingConv.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunction.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Type/MirType.h"

MirRegisterRef EzMirTestSuiteCallingConv::getFramePointerReg() const { return MirRegisterRef(); }

MirRegisterRef EzMirTestSuiteCallingConv::getStackPointerReg() const { return MirRegisterRef(); }

bool EzMirTestSuiteCallingConv::hasFramePointer(MirFunction *func) const { return false; }

bool EzMirTestSuiteCallingConv::canReturnInRegs(MirType *type) const { return false; }

ArgumentLocationDesc EzMirTestSuiteCallingConv::getArgLoc(MirType *type, CallLoweringState *callState)
{
    return ArgumentLocationDesc::Stack(0, nullptr);
}

ArgumentLocationDesc EzMirTestSuiteCallingConv::getReturnLoc(MirType *type, CallLoweringState *)
{
    return ArgumentLocationDesc::Stack(0, nullptr);
}

const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getAllCalleeSavedRegs() { return m_empty; }

const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getCalleeSavedRegs(MirRegisterClass *_class)
{
    return m_empty;
}

const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getAllCallerSavedRegs() { return m_empty; }

const std::pmr::vector<MirRegisterRef> &EzMirTestSuiteCallingConv::getCallerSavedRegs(MirRegisterClass *_class)
{
    return m_empty;
}