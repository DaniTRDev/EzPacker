#include "X86_64TargetDesc.h"
#include "X86_64FrameLowerer.h"
#include "X86_64RegisterAllocator.h"
#include "X86_64ElfBinaryDesc.h"
#include "X86_64CoffBinaryDesc.h"
#include "Builder/MirBuilderContext.h"
#include "Type/MirTypeTable.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Legalizer/MirLegalizer.h"
#include "x86_64CallingConvDesc.h"
#include "x86_64TargetInstructionTable.h"
#include "x86_64EncodingTable.h"
#include "x86_64LegalizerActionTable.h"
#include "X86_64TargetInstructionSelector.h"
#include "X86_64RelocationResolver.h"
#include "X86_64CodeEmitter.h"
#include "Instruction/MirTargetInstructionDesc.h"

namespace EzTargets::X86_64
{

/**
 * Records the builder context and sets up the PMR-backed bank/convention/binary registries.
 */
X86_64TargetDesc::X86_64TargetDesc(MirBuilderContext *ctx) :
    m_ctx(ctx), m_banks(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_convs(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_binaries(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource())
{
}

/**
 * Out-of-line destructor so translation units that only forward-declare the target's component
 * types (e.g. the resolver registration TU) do not need their complete definitions.
 */
X86_64TargetDesc::~X86_64TargetDesc() = default;

/**
 * Builds all x86-64 sub-components in dependency order: register banks/classes, calling
 * conventions, target instruction table, legalizer, selector, allocator, frame lowerer and
 * ELF/COFF binary descriptors.
 */
void X86_64TargetDesc::initialize()
{
    if (!m_ctx || m_initialized)
    {
        return;
    }
    m_initialized = true;

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
    m_gpr8 = classAlloc.new_object<MirRegisterClass>("GPR8", m_gprBank, alloc);

    m_gprBank->addClass("GPR64", m_gpr64);
    m_gprBank->addClass("GPR32", m_gpr32);
    m_gprBank->addClass("GPR16", m_gpr16);
    m_gprBank->addClass("GPR8", m_gpr8);

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
        { "rax", "eax", "ax", "al" },      // 0
        { "rcx", "ecx", "cx", "cl" },      // 1
        { "rdx", "edx", "dx", "dl" },      // 2
        { "rbx", "ebx", "bx", "bl" },      // 3
        { "rsp", "esp", "sp", "spl" },     // 4
        { "rbp", "ebp", "bp", "bpl" },     // 5
        { "rsi", "esi", "si", "sil" },     // 6
        { "rdi", "edi", "di", "dil" },     // 7
        { "r8", "r8d", "r8w", "r8b" },     // 8
        { "r9", "r9d", "r9w", "r9b" },     // 9
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
        "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
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
    EzTargets::X86_64::x86_64TargetInst::initializeTargetInstructionTable(this);

    // 7. Legalizer Info & Legalizer
    m_legalizerInfo = std::make_unique<x86_64LegalizerInfo>();
    m_legalizer = std::make_unique<MirLegalizer>(m_ctx, this);

    // 8. Instruction Selector
    m_isel = std::make_unique<X86_64TargetInstructionSelector>(this);

    // 9. Register Allocator & Frame Lowerer
    m_regAlloc = std::make_unique<X86_64RegisterAllocator>();
    m_frameLowerer = std::make_unique<X86_64FrameLowerer>();

    // 10. Binary Descriptors
    m_elfBinary = std::make_unique<X86_64ElfBinaryDesc>(alloc, m_isPic);
    m_elfBinary->initialize();

    m_coffBinary = std::make_unique<X86_64CoffBinaryDesc>(alloc);
    m_coffBinary->initialize();

    m_binaries.clear();
    m_binaries.push_back(m_elfBinary.get());
    m_binaries.push_back(m_coffBinary.get());
}

/// Returns the x86-64 frame lowerer created by initialize().
MirFrameLowerer *X86_64TargetDesc::getFrameLowerer() { return m_frameLowerer.get(); }

/// Returns the x86-64 instruction selector created by initialize().
MirInstructionSelector *X86_64TargetDesc::getInstructionSelector() { return m_isel.get(); }

/// Returns the 64-bit GPR class as the target's default integer register class.
MirRegisterClass *X86_64TargetDesc::getGprClass() { return m_gpr64; }

/// Returns the generic MIR legalizer driven by the x86-64 legality table.
MirLegalizer *X86_64TargetDesc::getLegalizer() { return m_legalizer.get(); }

/// Returns the x86-64 table-driven legality definitions.
LegalizerInfo *X86_64TargetDesc::getLegalizerInfo() { return m_legalizerInfo.get(); }

/// Returns the x86-64 graph-coloring register allocator.
MirRegisterAllocator *X86_64TargetDesc::getRegisterAllocator() { return m_regAlloc.get(); }

/// Memory displacements are 64-bit integers on x86-64.
MirType *X86_64TargetDesc::getMemOperandDisplacementType() { return m_ctx ? m_ctx->getTypeTable()->i64() : nullptr; }

/// Returns a pseudo register reference for RIP, encoded as GPR64 slot 16.
MirRegisterRef X86_64TargetDesc::getInstructionPtrReg() const
{
    return MirRegisterRef(m_gpr64, 16); // rip pseudo-ref
}

/// Maps a legality-table libcall symbol id to the runtime symbol name it should call.
std::string_view X86_64TargetDesc::getLibcallStr(uint8_t symId)
{
    // Delegate to the generated legality table so the id-space is the single source of truth.
    if (m_legalizerInfo)
    {
        return m_legalizerInfo->getLibcallSymbol(symId);
    }
    return {};
}

/// Returns the ELF and COFF binary descriptors registered for x86-64.
const std::pmr::vector<TargetBinaryDesc *> &X86_64TargetDesc::getAvailableBinaryDescriptors() { return m_binaries; }

/// Returns the System V and Win64 calling conventions registered for x86-64.
const std::pmr::vector<CallingConvDesc *> &X86_64TargetDesc::getAvailableCallingConventions() { return m_convs; }

/// Returns the GPR and FPR register banks registered for x86-64.
const std::pmr::vector<MirRegisterBank *> &X86_64TargetDesc::getAvailableRegisterBanks() { return m_banks; }

/**
 * Allocates a new register bank from the global allocator and registers it for later lookup.
 */
MirRegisterBank *X86_64TargetDesc::createRegisterBank(const char *name)
{
    if (!m_ctx || !name)
    {
        return nullptr;
    }

    auto *alloc = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirRegisterBank> bankAlloc(alloc);
    auto *bank = bankAlloc.new_object<MirRegisterBank>(name, alloc);
    m_banks.push_back(bank);
    return bank;
}

/**
 * Creates the x86-64 code emitter and installs a resolver that maps an instruction's encoding id
 * (falling back to its name) to the generated x86-64 encoding description.
 *
 * The target owns this binding so the shared emitter seam stays free of x86 encoding types.
 */
std::unique_ptr<GenericCodeEmitter> X86_64TargetDesc::createCodeEmitter()
{
    auto emitter = std::make_unique<EzTargets::X86_64::X86_64CodeEmitter>();
    emitter->setEncodingResolver(
            [](const MirTargetInstructionDesc *desc) -> const EzTargets::X86_64::EncodingDesc *
            {
                if (!desc)
                {
                    return nullptr;
                }
                if (const auto *enc = EzTargets::X86_64::getEncodingDesc(desc->getEncodingId()))
                {
                    return enc;
                }
                return EzTargets::X86_64::findEncodingDesc(desc->getName());
            });
    return emitter;
}

/// Lazily creates and returns the x86-64 relocation resolver.
TargetRelocationResolver *X86_64TargetDesc::getRelocationResolver()
{
    if (!m_relocResolver)
    {
        m_relocResolver = std::make_unique<X86_64RelocationResolver>();
    }
    return m_relocResolver.get();
}

} // namespace EzTargets::X86_64
