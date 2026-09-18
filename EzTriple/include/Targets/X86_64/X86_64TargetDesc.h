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
class MirAddressingModeMatcher;
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
    explicit X86_64TargetDesc(MirBuilderContext *ctx);
    ~X86_64TargetDesc() override = default;

    const char *getName() const override { return "x86_64"; }

    MirFrameLowerer *getFrameLowerer() override;
    MirInstructionSelector *getInstructionSelector() override;
    MirAddressingModeMatcher *getAddressingModeMatcher() override;
    MirRegisterClass *getGprClass() override;
    MirLegalizer *getLegalizer() override;
    LegalizerInfo *getLegalizerInfo() override;
    MirRegisterAllocator *getRegisterAllocator() override;
    MirType *getMemOperandDisplacementType() override;
    MirRegisterRef getInstructionPtrReg() const override;
    size_t getStackSlotSize() const override { return 8; }
    void initialize() override;
    std::string_view getLibcallStr(uint8_t symId) override;
    std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override;
    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override;
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override;

    CallingConvDesc *getSysVCallingConv() const { return m_sysVConv.get(); }
    CallingConvDesc *getWin64CallingConv() const { return m_win64Conv.get(); }
    TargetBinaryDesc *getElfBinaryDesc() const { return m_elfBinary.get(); }
    TargetBinaryDesc *getCoffBinaryDesc() const { return m_coffBinary.get(); }

  private:
    MirBuilderContext *m_ctx{ nullptr };

    MirRegisterBank *m_gprBank{ nullptr };
    MirRegisterBank *m_fprBank{ nullptr };

    MirRegisterClass *m_gpr64{ nullptr };
    MirRegisterClass *m_gpr32{ nullptr };
    MirRegisterClass *m_gpr16{ nullptr };
    MirRegisterClass *m_gpr8{ nullptr };
    MirRegisterClass *m_fpr64{ nullptr };
    MirRegisterClass *m_fpr32{ nullptr };

    std::unique_ptr<MirFrameLowerer> m_frameLowerer;
    std::unique_ptr<MirInstructionSelector> m_isel;
    std::unique_ptr<MirAddressingModeMatcher> m_modeMatcher;
    std::unique_ptr<MirLegalizer> m_legalizer;
    std::unique_ptr<LegalizerInfo> m_legalizerInfo;
    std::unique_ptr<MirRegisterAllocator> m_regAlloc;

    std::unique_ptr<CallingConvDesc> m_sysVConv;
    std::unique_ptr<CallingConvDesc> m_win64Conv;

    std::unique_ptr<TargetBinaryDesc> m_elfBinary;
    std::unique_ptr<TargetBinaryDesc> m_coffBinary;

    std::pmr::vector<MirRegisterBank *> m_banks;
    std::pmr::vector<CallingConvDesc *> m_convs;
    std::pmr::vector<TargetBinaryDesc *> m_binaries;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_TARGET_DESC_H
