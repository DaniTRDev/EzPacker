#include "CallingConvs/EzTestTripleCallingConv.h"

EzTestTripleCallingConv::EzTestTripleCallingConv(MirRegisterBank *gprBank,
                                                 MirRegisterBank *fprBank,
                                                 std::pmr::memory_resource *alloc) :
    m_gprBank(gprBank), m_fprBank(fprBank), m_alloc(alloc), m_allCalleeSaved(alloc), m_allCallerSaved(alloc),
    m_classCalleeSaved(alloc), m_classCallerSaved(alloc)
{
    m_gpr64 = m_gprBank->getClass("GPR64");
    m_gpr32 = m_gprBank->getClass("GPR32");
    m_gpr16 = m_gprBank->getClass("GPR16");
    m_gpr8 = m_gprBank->getClass("GPR8");

    m_fpr64 = m_fprBank->getClass("FPR64");
    m_fpr32 = m_fprBank->getClass("FPR32");

    initRegisterLists();
}

void EzTestTripleCallingConv::initRegisterLists()
{
    // Callee Saved GPRs: R3, RFP (R5), R12, R13, R14, R15
    const size_t calleeGprs[] = { EzTestTriple::RegisterIds::R3,  EzTestTriple::RegisterIds::RFP,
                                  EzTestTriple::RegisterIds::R12, EzTestTriple::RegisterIds::R13,
                                  EzTestTriple::RegisterIds::R14, EzTestTriple::RegisterIds::R15 };
    for (size_t id : calleeGprs)
    {
        m_allCalleeSaved.push_back(RegisterRef(m_gpr64, id));
    }

    // Caller Saved GPRs: R0, R1, R2, R6, R7, R8, R9, R10, R11
    const size_t callerGprs[] = { EzTestTriple::RegisterIds::R0, EzTestTriple::RegisterIds::R1,
                                  EzTestTriple::RegisterIds::R2, EzTestTriple::RegisterIds::R6,
                                  EzTestTriple::RegisterIds::R7, EzTestTriple::RegisterIds::R8,
                                  EzTestTriple::RegisterIds::R9, EzTestTriple::RegisterIds::R10,
                                  EzTestTriple::RegisterIds::R11 };
    for (size_t id : callerGprs)
    {
        m_allCallerSaved.push_back(RegisterRef(m_gpr64, id));
    }

    // All FPRs are Caller-Saved
    for (size_t id = 0; id < 8; ++id)
    {
        m_allCallerSaved.push_back(RegisterRef(m_fpr64, id));
    }
}

RegisterRef EzTestTripleCallingConv::getFramePointerReg() const
{
    return RegisterRef(m_gpr64, EzTestTriple::RegisterIds::RFP);
}

RegisterRef EzTestTripleCallingConv::getStackPointerReg() const
{
    return RegisterRef(m_gpr64, EzTestTriple::RegisterIds::RSP);
}

bool EzTestTripleCallingConv::hasFramePointer(MirFunction *func) const
{
    if (!func)
        return false;

    auto analysisData = func->getAnalysisData();
    return analysisData->m_hasCalls || analysisData->m_hasDynamicAllocs;
}

MirRegisterClass *EzTestTripleCallingConv::getGprClassForSize(size_t bytes) const
{
    if (bytes <= 1)
        return m_gpr8;
    if (bytes <= 2)
        return m_gpr16;
    if (bytes <= 4)
        return m_gpr32;
    return m_gpr64;
}

MirRegisterClass *EzTestTripleCallingConv::getFprClassForSize(size_t bytes) const
{
    if (bytes <= 4)
        return m_fpr32;
    return m_fpr64;
}

bool EzTestTripleCallingConv::canReturnInRegs(MirType *type) const
{
    if (!type)
        return false;
    const size_t totalBits = type->getTotalSizeInBits();

    // 128-bit primitives and aggregates can be returned across registers (R0:R2 or XMM0:XMM1)
    if (type->getKind() == MirTypeKind::Integer || type->getKind() == MirTypeKind::Pointer ||
        type->getKind() == MirTypeKind::FloatingPoint || type->getKind() == MirTypeKind::Class)
    {
        return totalBits <= 128;
    }

    return false;
}

ArgumentLocationDesc EzTestTripleCallingConv::getArgLoc(MirType *type, CallLoweringState *callState)
{
    const size_t totalBits = type->getTotalSizeInBits();
    const size_t sizeInBytes = (totalBits + 7) / 8;
    const bool isFloat = (type->getKind() == MirTypeKind::FloatingPoint);

    // -------------------------------------------------------------------------
    // 1. Floating-Point Types
    // -------------------------------------------------------------------------
    if (isFloat)
    {
        MirRegisterClass *targetClass = getFprClassForSize(sizeInBytes);
        RegisterRef reg;
        if (callState->allocate(targetClass, reg))
        {
            return ArgumentLocationDesc::Reg(reg, sizeInBytes);
        }
    }
    // -------------------------------------------------------------------------
    // 2. Structs & Aggregates
    // -------------------------------------------------------------------------
    else if (type->getKind() == MirTypeKind::Class)
    {
        if (sizeInBytes <= 16)
        {
            // Try splitting into GPRs (8-byte chunks)
            size_t regsNeeded = (sizeInBytes + 7) / 8;
            if (callState->getUsableRegCount(m_gpr64) - callState->getUsedRegCount(m_gpr64) >= regsNeeded)
            {
                std::vector<SplitPiece> pieces;
                size_t offset = 0;
                while (offset < sizeInBytes)
                {
                    RegisterRef pieceReg;
                    callState->allocate(m_gpr64, pieceReg);
                    pieces.push_back(SplitPiece{ .m_reg = pieceReg, .m_type = nullptr, .m_offsetInParam = offset });
                    offset += 8;
                }
                return ArgumentLocationDesc::Split(pieces);
            }
        }
        else
        {
            // Large aggregates pass indirectly via pointer
            RegisterRef ptrReg;
            if (callState->allocate(m_gpr64, ptrReg))
            {
                return ArgumentLocationDesc::Indirect(true, false, sizeInBytes, ptrReg);
            }
        }
    }
    // -------------------------------------------------------------------------
    // 3. 128-bit Wide Integers
    // -------------------------------------------------------------------------
    else if (totalBits > 64 && totalBits <= 128)
    {
        if (callState->getUsableRegCount(m_gpr64) - callState->getUsedRegCount(m_gpr64) >= 2)
        {
            RegisterRef loReg, hiReg;
            callState->allocate(m_gpr64, loReg);
            callState->allocate(m_gpr64, hiReg);

            std::vector<SplitPiece> pieces = { SplitPiece{ .m_reg = loReg, .m_type = nullptr, .m_offsetInParam = 0 },
                                               SplitPiece{ .m_reg = hiReg, .m_type = nullptr, .m_offsetInParam = 8 } };
            return ArgumentLocationDesc::Split(pieces);
        }
    }
    // -------------------------------------------------------------------------
    // 4. Standard Integers & Pointers
    // -------------------------------------------------------------------------
    else
    {
        MirRegisterClass *targetClass = getGprClassForSize(sizeInBytes);
        RegisterRef reg;
        if (callState->allocate(targetClass, reg))
        {
            return ArgumentLocationDesc::Reg(reg, sizeInBytes);
        }
    }

    // -------------------------------------------------------------------------
    // Fallback: Stack Allocation
    // -------------------------------------------------------------------------
    StackFrameObject *stackObj = callState->allocateStack(type);
    return ArgumentLocationDesc::Stack(sizeInBytes, stackObj);
}

ArgumentLocationDesc EzTestTripleCallingConv::getReturnLoc(MirType *type, CallLoweringState *)
{
    const size_t totalBits = type->getTotalSizeInBits();
    const size_t sizeInBytes = (totalBits + 7) / 8;

    // Floating-Point Return (XMM0)
    if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        MirRegisterClass *targetClass = getFprClassForSize(sizeInBytes);
        return ArgumentLocationDesc::Reg(RegisterRef(targetClass, EzTestTriple::RegisterIds::XMM0), sizeInBytes);
    }

    // 128-bit Wide Primitives & Small Structs -> Split across R0:R2
    if (totalBits > 64 && totalBits <= 128)
    {
        std::vector<SplitPiece> pieces = { SplitPiece{ .m_reg = RegisterRef(m_gpr64, EzTestTriple::RegisterIds::R0),
                                                       .m_type = nullptr,
                                                       .m_offsetInParam = 0 },
                                           SplitPiece{ .m_reg = RegisterRef(m_gpr64, EzTestTriple::RegisterIds::R2),
                                                       .m_type = nullptr,
                                                       .m_offsetInParam = 8 } };
        return ArgumentLocationDesc::Split(pieces);
    }

    // Standard 8/16/32/64-bit Integer/Pointer Return (R0)
    MirRegisterClass *targetClass = getGprClassForSize(sizeInBytes);
    return ArgumentLocationDesc::Reg(RegisterRef(targetClass, EzTestTriple::RegisterIds::R0), sizeInBytes);
}

const std::pmr::vector<RegisterRef> &EzTestTripleCallingConv::getAllCalleeSavedRegs() { return m_allCalleeSaved; }

const std::pmr::vector<RegisterRef> &EzTestTripleCallingConv::getCalleeSavedRegs(MirRegisterClass *_class)
{
    m_classCalleeSaved.clear();
    for (const auto &reg : m_allCalleeSaved)
    {
        if (reg.getClass() == _class)
            m_classCalleeSaved.push_back(reg);
    }
    return m_classCalleeSaved;
}

const std::pmr::vector<RegisterRef> &EzTestTripleCallingConv::getAllCallerSavedRegs() { return m_allCallerSaved; }

const std::pmr::vector<RegisterRef> &EzTestTripleCallingConv::getCallerSavedRegs(MirRegisterClass *_class)
{
    m_classCallerSaved.clear();
    for (const auto &reg : m_allCallerSaved)
    {
        if (reg.getClass() == _class)
            m_classCallerSaved.push_back(reg);
    }
    return m_classCallerSaved;
}