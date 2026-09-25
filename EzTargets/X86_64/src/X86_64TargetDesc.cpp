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
#include "x86_64RegisterInfo.h"

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
    m_extensions.registerExtension("sse", "Streaming SIMD Extensions", true);
    m_extensions.registerExtension("sse2", "Streaming SIMD Extensions 2", true, { "sse" });
    m_extensions.registerExtension("sse3", "Streaming SIMD Extensions 3", false, { "sse2" });
    m_extensions.registerExtension("ssse3", "Supplemental Streaming SIMD Extensions 3", false, { "sse3" });
    m_extensions.registerExtension("sse4a", "AMD Streaming SIMD Extensions 4a", false, { "sse3" });
    m_extensions.registerExtension("sse4_1", "Streaming SIMD Extensions 4.1", false, { "ssse3" });
    m_extensions.registerExtension("sse4.1", "Streaming SIMD Extensions 4.1", false, { "sse4_1" });
    m_extensions.registerExtension("sse4_2", "Streaming SIMD Extensions 4.2", false, { "sse4_1" });
    m_extensions.registerExtension("sse4.2", "Streaming SIMD Extensions 4.2", false, { "sse4_2" });
    m_extensions.registerExtension("avx", "Advanced Vector Extensions", false, { "sse4_2" });
    m_extensions.registerExtension("avx2", "Advanced Vector Extensions 2", false, { "avx" });
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

    // 1-4. Register Banks and Classes
    m_banks = EzTargets::TableGen::x86_64::initializeRegisterBanks(alloc);
    for (auto *b : m_banks)
    {
        if (std::string_view(b->getName()) == "GPR")
        {
            m_gprBank = b;
            m_gpr64 = b->getClass("GPR64");
            m_gpr32 = b->getClass("GPR32");
            m_gpr16 = b->getClass("GPR16");
            m_gpr8 = b->getClass("GPR8");
        }
        else if (std::string_view(b->getName()) == "FPR")
        {
            m_fprBank = b;
            m_fpr64 = b->getClass("FPR64");
            m_fpr32 = b->getClass("FPR32");
            m_vr128 = b->getClass("VR128");
        }
    }

    // 5. Calling Conventions
    m_sysVConv = std::make_unique<SysV_AMD64CallingConvDesc>(m_ctx, m_gpr64, m_fpr64);
    m_win64Conv = std::make_unique<Win64CallingConvDesc>(m_ctx, m_gpr64, m_fpr64);

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

/// Returns a pseudo register reference for RIP, retrieved from the generated register metadata.
MirRegisterRef X86_64TargetDesc::getInstructionPtrReg() const
{
    return MirRegisterRef(m_gpr64, EzTargets::TableGen::x86_64::getSpecialRegId("rip"));
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
