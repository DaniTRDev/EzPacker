#include "TestCallingConvention.h"

TestCallingConvention::TestCallingConvention()
{
    // Populate distinct, constrained register pools for robust stress-testing.
    // GPRs: 1, 2 (Caller-saved/Volatile), 3 (Callee-saved/Preserved)
    m_gprCallerSaved = { PhysicalRegId(1), PhysicalRegId(2) };
    m_gprCalleeSaved = { PhysicalRegId(3) };

    // FPRs: 4 (Caller-saved/Volatile), 5 (Callee-saved/Preserved)
    m_fprCallerSaved = { PhysicalRegId(4) };
    m_fprCalleeSaved = { PhysicalRegId(5) };
}

const char *TestCallingConvention::getName() const { return "TestCallingConvention"; }

ArgumentLocationDesc TestCallingConvention::getArgLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeBytes = type->getTotalSizeInBytes();
    size_t alignment = type->getMaxAlignmentInBytes();

    // Rule 1: Test Split structural logic.
    // Force any 64-bit value to split into two 32-bit registers.
    if (sizeBytes == 8)
    {
        PhysicalRegId regLo, regHi;
        if (callState->allocateGpr(regLo) && callState->allocateGpr(regHi))
        {
            std::vector<SplitPiece> pieces = {
                { regLo, 4, 0 }, // Low 32 bits at offset 0
                { regHi, 4, 4 }  // High 32 bits at offset 4
            };
            return ArgumentLocationDesc::Split(pieces);
        }
    }

    // Rule 2: Alternating Register vs Stack Test
    // If the current total used register count is even, force this argument onto the stack
    // even if hardware registers are completely free.
    size_t totalAllocatedRegs = callState->getUsedGprCount() + callState->getUsedFprCount();
    if (totalAllocatedRegs % 2 == 0)
    {
        int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
        return ArgumentLocationDesc::Stack(offset, sizeBytes);
    }

    // Rule 3: Standard Allocation Fallback
    if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        PhysicalRegId fpr;
        if (callState->allocateFpr(fpr))
        {
            return ArgumentLocationDesc::Reg(fpr, sizeBytes);
        }
    }
    else
    {
        PhysicalRegId gpr;
        if (callState->allocateGpr(gpr))
        {
            return ArgumentLocationDesc::Reg(gpr, sizeBytes);
        }
    }

    // Exhausted registers -> spill to stack slot
    int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
    return ArgumentLocationDesc::Stack(offset, sizeBytes);
}

ArgumentLocationDesc TestCallingConvention::getReturnLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeBytes = type->getTotalSizeInBytes();

    if (!canReturnInRegs(type))
    {
        // Indirect SRET pointer assignment: Expect it in GPR 1 (matching pool boundary)
        return ArgumentLocationDesc::Indirect(true, sizeBytes, PhysicalRegId(1));
    }

    if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        return ArgumentLocationDesc::Reg(PhysicalRegId(4), sizeBytes); // Fixed return FPR (Volatile Pool)
    }

    return ArgumentLocationDesc::Reg(PhysicalRegId(1), sizeBytes); // Fixed return GPR (Volatile Pool)
}

bool TestCallingConvention::canReturnInRegs(MirType *type) const
{
    // Force types larger than 64 bits to lower via an explicit Struct Return (SRET)
    return type->getTotalSizeInBits() <= 64;
}

bool TestCallingConvention::isCalleeCleanup() const { return false; }

size_t TestCallingConvention::getStackAlignment() const { return 32; }
size_t TestCallingConvention::getShadowSpaceSize() const { return 24; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCalleeSavedGPRegs() const { return m_gprCalleeSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCalleeSavedFPRegs() const { return m_fprCalleeSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCallerSavedGPRegs() const { return m_gprCallerSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCallerSavedFPRegs() const { return m_fprCallerSaved; }