#include "TestCallingConvention.h"

TestCallingConvention::TestCallingConvention(MirBuilderContext *ctx) : m_ctx(ctx)
{
    m_calleeSavedRegs = std::pmr::unordered_map<RegisterRefClass, std::pmr::vector<RegisterRef>>(
            { { RegisterRefClass::GPR,
                std::pmr::vector<RegisterRef>({ RegisterRef::preg(RegisterRefClass::GPR, 3) },
                                              m_ctx->getGlobalAllocator()) },
              { RegisterRefClass::FPR,
                std::pmr::vector<RegisterRef>({ RegisterRef::preg(RegisterRefClass::FPR, 5) },
                                              m_ctx->getGlobalAllocator()) } },
            0,
            m_ctx->getGlobalAllocator());

    m_callerSavedRegs = std::pmr::unordered_map<RegisterRefClass, std::pmr::vector<RegisterRef>>(
            { { RegisterRefClass::GPR,
                std::pmr::vector<RegisterRef>(
                        { RegisterRef::preg(RegisterRefClass::GPR, 1), RegisterRef::preg(RegisterRefClass::GPR, 2) },
                        m_ctx->getGlobalAllocator()) },
              { RegisterRefClass::FPR,
                std::pmr::vector<RegisterRef>({ RegisterRef::preg(RegisterRefClass::FPR, 4) },
                                              m_ctx->getGlobalAllocator()) } },
            0,
            m_ctx->getGlobalAllocator());
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
        RegisterRef regLo, regHi;
        if (callState->allocate(RegisterRefClass::GPR, regLo) && callState->allocate(RegisterRefClass::GPR, regHi))
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
        RegisterRef reg;
        if (callState->allocate(RegisterRefClass::GPR, reg))
        {
            return ArgumentLocationDesc::Reg(reg, sizeBytes);
        }
    }
    // C. 256-bit values (32 bytes): Forced to Indirect pass-by-value.
    // Passes a pointer in a GPR if available; otherwise allocates a stack slot for the pointer address.
    else if (sizeBytes == 32)
    {
        RegisterRef ptrDest;
        if (callState->allocate(RegisterRefClass::GPR, ptrDest))
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
    size_t totalAllocatedRegs =
            callState->getUsedRegCount(RegisterRefClass::GPR) + callState->getUsedRegCount(RegisterRefClass::FPR);
    if (totalAllocatedRegs % 2 == 0)
    {
        int64_t offset = callState->allocateStackSlot(sizeBytes, alignment);
        return ArgumentLocationDesc::Stack(offset, sizeBytes);
    }

    // -------------------------------------------------------------------------
    // Rule 3: Standard Register Allocation Fallback
    // -------------------------------------------------------------------------
    RegisterRefClass targetClass =
            (type->getKind() == MirTypeKind::FloatingPoint) ? RegisterRefClass::FPR : RegisterRefClass::GPR;

    RegisterRef reg;
    if (callState->allocate(targetClass, reg))
    {
        return ArgumentLocationDesc::Reg(reg, sizeBytes);
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
        return ArgumentLocationDesc::Indirect(true, true, sizeBytes, RegisterRef::preg(RegisterRefClass::GPR, 1));
    }

    // Floating-point scalar returns -> Volatile FPR 4
    if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        return ArgumentLocationDesc::Reg(RegisterRef::preg(RegisterRefClass::FPR, 4), sizeBytes);
    }

    // All other scalar returns (GPR) -> Volatile GPR 1
    return ArgumentLocationDesc::Reg(RegisterRef::preg(RegisterRefClass::GPR, 1), sizeBytes);
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

const std::pmr::vector<RegisterRef> &TestCallingConvention::getCalleeSavedRegs(RegisterRefClass refClass) const
{
    if (auto it = m_calleeSavedRegs.find(refClass); it != m_calleeSavedRegs.end())
        return it->second;

    throw std::out_of_range("Invalid callee saved register class");
}

const std::pmr::vector<RegisterRef> &TestCallingConvention::getCallerSavedRegs(RegisterRefClass refClass) const
{
    if (auto it = m_callerSavedRegs.find(refClass); it != m_callerSavedRegs.end())
        return it->second;

    throw std::out_of_range("Invalid caller saved register class");
}
