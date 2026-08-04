#include "TestCallingConvention.h"

TestCallingConvention::TestCallingConvention()
{
    // Populate distinct, highly constrained register pools for stress-testing lowering passes.
    // Volatile GPR pool: {1, 2}, Preserved GPR pool: {3}
    m_gprCallerSaved = { PhysicalRegId(1), PhysicalRegId(2) };
    m_gprCalleeSaved = { PhysicalRegId(3) };

    // Volatile FPR pool: {4}, Preserved FPR pool: {5}
    m_fprCallerSaved = { PhysicalRegId(4) };
    m_fprCalleeSaved = { PhysicalRegId(5) };
}

const char *TestCallingConvention::getName() const { return "TestCallingConvention"; }

ArgumentLocationDesc TestCallingConvention::getArgLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeBytes = type->getTotalSizeInBytes();
    size_t alignment = type->getMaxAlignmentInBytes();

    // -------------------------------------------------------------------------
    // Rule 1: Size-Specific Overrides
    // -------------------------------------------------------------------------

    // A. 64-bit values (8 bytes): Forced to split into two 32-bit GPRs
    if (sizeBytes == 8)
    {
        PhysicalRegId regLo, regHi;
        if (callState->allocateGpr(regLo) && callState->allocateGpr(regHi))
        {
            std::vector<SplitPiece> pieces = {
                { regLo, type->getOwner()->i32(), 0 }, // Low 32 bits at offset 0
                { regHi, type->getOwner()->i32(), 4 }  // High 32 bits at offset 4
            };
            return ArgumentLocationDesc::Split(pieces);
        }
    }
    // B. 32-bit values (4 bytes): Directly assigned to a single GPR if available
    else if (sizeBytes == 4)
    {
        PhysicalRegId reg;
        if (callState->allocateGpr(reg))
        {
            return ArgumentLocationDesc::Reg(reg, sizeBytes);
        }
    }
    // C. 256-bit values (32 bytes): Forced to Indirect pass-by-value.
    // Passes a pointer in a GPR if available; otherwise allocates a stack slot for the pointer address.
    else if (sizeBytes == 32)
    {
        PhysicalRegId ptrDest;
        if (callState->allocateGpr(ptrDest))
        {
            return ArgumentLocationDesc::Indirect(true, false, sizeBytes, ptrDest);
        }
        else
        {
            int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
            return ArgumentLocationDesc::Stack(offset, sizeBytes);
        }
    }

    // -------------------------------------------------------------------------
    // Rule 2: Alternating Parity Interleaving Rule
    // -------------------------------------------------------------------------
    // If the total count of currently allocated registers (GPRs + FPRs) is EVEN,
    // force this parameter to the stack to test non-contiguous register utilization routines.
    size_t totalAllocatedRegs = callState->getUsedGprCount() + callState->getUsedFprCount();
    if (totalAllocatedRegs % 2 == 0)
    {
        int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
        return ArgumentLocationDesc::Stack(offset, sizeBytes);
    }

    // -------------------------------------------------------------------------
    // Rule 3: Standard Register Allocation Fallback
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // Rule 4: Exhaustion Spill Fallback
    // -------------------------------------------------------------------------
    // Fallback when register pools are full: allocate an aligned outgoing stack parameter slot.
    int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
    return ArgumentLocationDesc::Stack(offset, sizeBytes);
}

ArgumentLocationDesc TestCallingConvention::getReturnLoc(MirType *type, CallLoweringState *callState)
{
    size_t sizeBytes = type->getTotalSizeInBytes();

    // Types exceeding 64 bits cannot be returned in registers.
    // Lower as an Indirect SRET pointer (byVal=true, copyOnReg=true) passed in GPR 1.
    if (!canReturnInRegs(type))
    {
        return ArgumentLocationDesc::Indirect(true, true, sizeBytes, PhysicalRegId(1));
    }

    // Floating-point scalar returns -> Volatile FPR 4
    if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        return ArgumentLocationDesc::Reg(PhysicalRegId(4), sizeBytes);
    }

    // All other scalar returns (GPR) -> Volatile GPR 1
    return ArgumentLocationDesc::Reg(PhysicalRegId(1), sizeBytes);
}

bool TestCallingConvention::canReturnInRegs(MirType *type) const
{
    // Force types larger than 64 bits to lower via explicit Struct Return (SRET)
    return type->getTotalSizeInBits() <= 64;
}

bool TestCallingConvention::isCalleeCleanup() const
{
    return false; // Caller-managed stack argument cleanup
}

size_t TestCallingConvention::getStackAlignment() const
{
    return 32; // Enforce 32-byte alignment boundary before CALL
}

size_t TestCallingConvention::getShadowSpaceSize() const
{
    return 24; // Allocate 24 bytes of shadow/home area
}

const std::vector<PhysicalRegId> &TestCallingConvention::getCalleeSavedGPRegs() const { return m_gprCalleeSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCalleeSavedFPRegs() const { return m_fprCalleeSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCallerSavedGPRegs() const { return m_gprCallerSaved; }

const std::vector<PhysicalRegId> &TestCallingConvention::getCallerSavedFPRegs() const { return m_fprCallerSaved; }