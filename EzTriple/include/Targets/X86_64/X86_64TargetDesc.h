#ifndef EZTRIPLE_X86_64_TARGET_DESC_H
#define EZTRIPLE_X86_64_TARGET_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Operand/MirRegisterReference.h"
#include <memory>
#include <vector>

class MirBuilderContext;
class MirFrameLowerer;
class MirInstructionSelector;
class MirRegisterClass;
class MirRegisterBank;
class MirLegalizer;
class LegalizerInfo;
class MirRegisterAllocator;
class MirType;
class CallingConvDesc;
class TargetBinaryDesc;

namespace EzTriple
{

/**
 * Concrete Target Descriptor for the x86-64 (AMD64) architecture.
 * Manages GPR and FPR register banks, calling conventions (System V AMD64 and Windows x64),
 * table-driven legalization, tree-pattern instruction selection, graph-coloring register allocation,
 * and frame lowering.
 */
class X86_64TargetDesc : public TargetDesc
{
  public:
    /**
     * Creates the descriptor bound to the shared builder context used by its sub-components.
     */
    explicit X86_64TargetDesc(MirBuilderContext *ctx);
    ~X86_64TargetDesc() override;

    /// Identifier used for diagnostics and target lookup.
    const char *getName() const override { return "x86_64"; }

    /// Returns the x86-64 frame lowerer, creating it on first access.
    MirFrameLowerer *getFrameLowerer() override;

    /// Returns the x86-64 instruction selector, creating it on first access.
    MirInstructionSelector *getInstructionSelector() override;

    /// Returns the 64-bit general-purpose register class used as the default integer class.
    MirRegisterClass *getGprClass() override;

    /// Returns the x86-64 legalizer, creating it on first access.
    MirLegalizer *getLegalizer() override;

    /// Returns the table-driven legality information shared with the legalizer.
    LegalizerInfo *getLegalizerInfo() override;

    /// Returns the x86-64 graph-coloring register allocator.
    MirRegisterAllocator *getRegisterAllocator() override;

    /// Returns the type used for a memory operand's displacement (pointer-sized integer).
    MirType *getMemOperandDisplacementType() override;

    /// Returns a register reference to the instruction pointer (RIP) for PC-relative addressing.
    MirRegisterRef getInstructionPtrReg() const override;

    /// Stack slots are 8 bytes on x86-64.
    size_t getStackSlotSize() const override { return 8; }

    /// Builds all register banks, classes, conventions and binary descriptors for the target.
    void initialize() override;

    /// Maps a libcall symbol id to its runtime symbol name (e.g. the id for __divdi3).
    std::string_view getLibcallStr(uint8_t symId) override;

    /// Returns the ELF and COFF binary descriptors available for x86-64.
    const std::pmr::vector<TargetBinaryDesc *> &getAvailableBinaryDescriptors() override;

    /// Returns the System V and Win64 calling conventions defined for this target.
    const std::pmr::vector<CallingConvDesc *> &getAvailableCallingConventions() override;

    /// Returns the GPR and FPR register banks exposed by this target.
    const std::pmr::vector<MirRegisterBank *> &getAvailableRegisterBanks() override;

    /// Creates and registers a new register bank owned by this target's allocator.
    MirRegisterBank *createRegisterBank(const char *name) override;

    /// Creates the x86-64 machine code emitter.
    std::unique_ptr<GenericCodeEmitter> createCodeEmitter() override;

    /// Returns the x86-64 relocation resolver used to patch encoded branch fields.
    TargetRelocationResolver *getRelocationResolver() override;

    /**
     * Records whether position-independent code was requested. Must be called before initialize()
     * so the ELF binary descriptor is constructed with the right PIC mode.
     */
    void setPositionIndependent(bool isPositionIndependent) { m_isPic = isPositionIndependent; }

    /// Returns the System V AMD64 calling convention instance.
    CallingConvDesc *getSysVCallingConv() const { return m_sysVConv.get(); }

    /// Returns the Windows x64 calling convention instance.
    CallingConvDesc *getWin64CallingConv() const { return m_win64Conv.get(); }

    /// Returns the ELF binary descriptor instance.
    TargetBinaryDesc *getElfBinaryDesc() const { return m_elfBinary.get(); }

    /// Returns the COFF binary descriptor instance.
    TargetBinaryDesc *getCoffBinaryDesc() const { return m_coffBinary.get(); }

  private:
    MirBuilderContext *m_ctx{ nullptr }; ///< Shared builder context passed to generated components.
    bool m_initialized{ false };         ///< Guards initialize() so it only builds components once.

    MirRegisterBank *m_gprBank{ nullptr }; ///< General-purpose integer register bank.
    MirRegisterBank *m_fprBank{ nullptr }; ///< Floating-point/vector register bank.

    MirRegisterClass *m_gpr64{ nullptr }; ///< 64-bit general-purpose register class.
    MirRegisterClass *m_gpr32{ nullptr }; ///< 32-bit general-purpose register class.
    MirRegisterClass *m_gpr16{ nullptr }; ///< 16-bit general-purpose register class.
    MirRegisterClass *m_gpr8{ nullptr };  ///< 8-bit general-purpose register class.
    MirRegisterClass *m_fpr64{ nullptr }; ///< 64-bit floating-point register class.
    MirRegisterClass *m_fpr32{ nullptr }; ///< 32-bit floating-point register class.

    std::unique_ptr<MirFrameLowerer> m_frameLowerer;  ///< Lazily created frame lowerer.
    std::unique_ptr<MirInstructionSelector> m_isel;   ///< Lazily created instruction selector.
    std::unique_ptr<MirLegalizer> m_legalizer;        ///< Lazily created legalizer.
    std::unique_ptr<LegalizerInfo> m_legalizerInfo;          ///< Table-driven legality definitions.
    std::unique_ptr<MirRegisterAllocator> m_regAlloc;        ///< Graph-coloring register allocator.

    std::unique_ptr<CallingConvDesc> m_sysVConv;  ///< System V AMD64 calling convention.
    std::unique_ptr<CallingConvDesc> m_win64Conv; ///< Windows x64 calling convention.

    std::unique_ptr<TargetBinaryDesc> m_elfBinary;  ///< ELF binary descriptor.
    std::unique_ptr<TargetBinaryDesc> m_coffBinary; ///< COFF binary descriptor.

    std::unique_ptr<TargetRelocationResolver> m_relocResolver; ///< Branch/field relocation patcher.

    bool m_isPic{ false }; ///< Whether -fPIC was requested before initialize().

    std::pmr::vector<MirRegisterBank *> m_banks;     ///< Registered register banks.
    std::pmr::vector<CallingConvDesc *> m_convs;     ///< Registered calling conventions.
    std::pmr::vector<TargetBinaryDesc *> m_binaries; ///< Registered binary descriptors.
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_TARGET_DESC_H
