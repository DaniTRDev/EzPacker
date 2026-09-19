#include "Targets/X86_64/X86_64TargetDesc.h"
#include "Targets/X86_64/X86_64FrameLowerer.h"
#include "Targets/X86_64/X86_64RegisterAllocator.h"
#include "Targets/X86_64/X86_64ElfBinaryDesc.h"
#include "Targets/X86_64/X86_64CoffBinaryDesc.h"
#include "InstructionSelector/MirAddressingModeMatcher.h"
#include "Builder/MirBuilderContext.h"
#include "Type/MirTypeTable.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Legalizer/MirLegalizer.h"
#include "x86_64CallingConvDesc.h"
#include "x86_64TargetInstructionTable.h"
#include "x86_64LegalizerActionTable.h"
#include "Targets/X86_64/X86_64InstructionSelector.h"

namespace EzTriple
{

X86_64TargetDesc::X86_64TargetDesc(MirBuilderContext *ctx) :
    m_ctx(ctx),
    m_banks(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_convs(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_binaries(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource())
{
}

void X86_64TargetDesc::initialize()
{
    if (!m_ctx)
    {
        return;
    }

    auto *alloc = m_ctx->getGlobalAllocator();

    // 1. Create Register Banks
    std::pmr::polymorphic_allocator<MirRegisterBank> bankAlloc(alloc);
    std::pmr::polymorphic_allocator<MirRegisterClass> classAlloc(alloc);

    m_gprBank = bankAlloc.new_object<MirRegisterBank>("GPR", alloc);
    m_fprBank = bankAlloc.new_object<MirRegisterBank>("FPR", alloc);

    // 2. Create Register Classes
    m_gpr64 = classAlloc.new_object<MirRegisterClass>("GPR64", m_gprBank, alloc);
    m_gpr32 = classAlloc.new_object<MirRegisterClass>("GPR32", m_gprBank, alloc);
    m_gpr16 = classAlloc.new_object<MirRegisterClass>("GPR16", m_gprBank, alloc);
    m_gpr8  = classAlloc.new_object<MirRegisterClass>("GPR8",  m_gprBank, alloc);

    m_gprBank->addClass("GPR64", m_gpr64);
    m_gprBank->addClass("GPR32", m_gpr32);
    m_gprBank->addClass("GPR16", m_gpr16);
    m_gprBank->addClass("GPR8",  m_gpr8);

    m_fpr64 = classAlloc.new_object<MirRegisterClass>("FPR64", m_fprBank, alloc);
    m_fpr32 = classAlloc.new_object<MirRegisterClass>("FPR32", m_fprBank, alloc);

    m_fprBank->addClass("FPR64", m_fpr64);
    m_fprBank->addClass("FPR32", m_fpr32);

    m_banks.clear();
    m_banks.push_back(m_gprBank);
    m_banks.push_back(m_fprBank);

    // 3. Register GPRs in exact hardware encoding order (0..15):
    //    RAX=0, RCX=1, RDX=2, RBX=3, RSP=4, RBP=5, RSI=6, RDI=7,
    //    R8=8,  R9=9,  R10=10, R11=11, R12=12, R13=13, R14=14, R15=15
    struct GprDef
    {
        std::string_view name64;
        std::string_view name32;
        std::string_view name16;
        std::string_view name8;
    };

    static constexpr GprDef s_gprs[] = {
        { "rax", "eax", "ax", "al"  }, // 0
        { "rcx", "ecx", "cx", "cl"  }, // 1
        { "rdx", "edx", "dx", "dl"  }, // 2
        { "rbx", "ebx", "bx", "bl"  }, // 3
        { "rsp", "esp", "sp", "spl" }, // 4
        { "rbp", "ebp", "bp", "bpl" }, // 5
        { "rsi", "esi", "si", "sil" }, // 6
        { "rdi", "edi", "di", "dil" }, // 7
        { "r8",  "r8d", "r8w", "r8b" }, // 8
        { "r9",  "r9d", "r9w", "r9b" }, // 9
        { "r10", "r10d", "r10w", "r10b" }, // 10
        { "r11", "r11d", "r11w", "r11b" }, // 11
        { "r12", "r12d", "r12w", "r12b" }, // 12
        { "r13", "r13d", "r13w", "r13b" }, // 13
        { "r14", "r14d", "r14w", "r14b" }, // 14
        { "r15", "r15d", "r15w", "r15b" }, // 15
    };

    for (const auto &gpr : s_gprs)
    {
        m_gpr8->addRegister(gpr.name8, 8, 0, {});
        m_gpr16->addRegister(gpr.name16, 16, 0, { m_gpr8->getReg(gpr.name8) });
        m_gpr32->addRegister(gpr.name32, 32, 0, { m_gpr16->getReg(gpr.name16) });
        m_gpr64->addRegister(gpr.name64, 64, 0, { m_gpr32->getReg(gpr.name32) });
    }

    // 4. Register FPRs (xmm0..xmm15)
    static constexpr std::string_view s_xmms[] = {
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
    };
    for (const auto &xmm : s_xmms)
    {
        m_fpr32->addRegister(xmm, 32, 0, {});
        m_fpr64->addRegister(xmm, 64, 0, { m_fpr32->getReg(xmm) });
    }

    // 5. Calling Conventions
    m_sysVConv = std::make_unique<SysV_AMD64CallingConvDesc>(m_ctx, m_gpr64);
    m_win64Conv = std::make_unique<Win64CallingConvDesc>(m_ctx, m_gpr64);

    m_convs.clear();
    m_convs.push_back(m_sysVConv.get());
    m_convs.push_back(m_win64Conv.get());

    // 6. Target Instruction Descriptors Table
    EzTriple::x86_64TargetInst::initializeTargetInstructionTable(this);

    // 7. Legalizer Info & Legalizer
    m_legalizerInfo = std::make_unique<x86_64LegalizerInfo>();
    m_legalizer = std::make_unique<MirLegalizer>(m_ctx, this);

    // 8. Instruction Selector & Addressing Mode Matcher
    m_isel = std::make_unique<X86_64TargetInstructionSelector>(this);
    m_modeMatcher = std::make_unique<X86AddressingModeMatcher>();

    // 9. Register Allocator & Frame Lowerer
    m_regAlloc = std::make_unique<X86_64RegisterAllocator>();
    m_frameLowerer = std::make_unique<X86_64FrameLowerer>();

    // 10. Binary Descriptors
    m_elfBinary = std::make_unique<X86_64ElfBinaryDesc>(alloc);
    m_elfBinary->initialize();

    m_coffBinary = std::make_unique<X86_64CoffBinaryDesc>(alloc);
    m_coffBinary->initialize();

    m_binaries.clear();
    m_binaries.push_back(m_elfBinary.get());
    m_binaries.push_back(m_coffBinary.get());
}

MirFrameLowerer *X86_64TargetDesc::getFrameLowerer()
{
    return m_frameLowerer.get();
}

MirInstructionSelector *X86_64TargetDesc::getInstructionSelector()
{
    return m_isel.get();
}

MirAddressingModeMatcher *X86_64TargetDesc::getAddressingModeMatcher()
{
    return m_modeMatcher.get();
}

MirRegisterClass *X86_64TargetDesc::getGprClass()
{
    return m_gpr64;
}

MirLegalizer *X86_64TargetDesc::getLegalizer()
{
    return m_legalizer.get();
}

LegalizerInfo *X86_64TargetDesc::getLegalizerInfo()
{
    return m_legalizerInfo.get();
}

MirRegisterAllocator *X86_64TargetDesc::getRegisterAllocator()
{
    return m_regAlloc.get();
}

MirType *X86_64TargetDesc::getMemOperandDisplacementType()
{
    return m_ctx ? m_ctx->getTypeTable()->i64() : nullptr;
}

MirRegisterRef X86_64TargetDesc::getInstructionPtrReg() const
{
    return MirRegisterRef(m_gpr64, 16); // rip pseudo-ref
}

std::string_view X86_64TargetDesc::getLibcallStr(uint8_t symId)
{
    switch (symId)
    {
        case 0: return "__returnNothing";
        case 1: return "__divdi3";
        case 2: return "__udivdi3";
        case 3: return "__moddi3";
        case 4: return "__umoddi3";
        case 5: return "__muldi3";
        default: return {};
    }
}

std::pmr::vector<TargetBinaryDesc *> X86_64TargetDesc::getAvailableBinaryDescriptors()
{
    return m_binaries;
}

std::pmr::vector<CallingConvDesc *> X86_64TargetDesc::getAvailableCallingConventions()
{
    return m_convs;
}

std::pmr::vector<MirRegisterBank *> X86_64TargetDesc::getAvailableRegisterBanks()
{
    return m_banks;
}

} // namespace EzTriple
