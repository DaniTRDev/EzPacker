#ifndef EZTRIPLE_MIR_ADDRESSING_MODE_MATCHER_H
#define EZTRIPLE_MIR_ADDRESSING_MODE_MATCHER_H

#include "EzTripleCommon.h"
#include <cstdint>
#include <vector>

class MirBuilderContext;
class MirOperand;
class MirRegister;
class MirInstruction;
class MirInstructionSelector;

struct MatchedAddressingMode
{
    MirRegister *m_base{ nullptr };
    int64_t m_disp{ 0 };
    MirRegister *m_index{ nullptr };
    uint8_t m_scale{ 1 };
    std::vector<MirInstruction *> m_foldedInstructions;
};

class MirAddressingModeMatcher
{
  public:
    virtual ~MirAddressingModeMatcher() = default;

    /**
     * Matches an address operand (register or pointer computation tree) into hardware addressing mode.
     */
    virtual bool matchAddress(MirBuilderContext *ctx,
                              MirOperand *addrOp,
                              MatchedAddressingMode &outMode) = 0;
};

/**
 * Concrete addressing mode matcher for x86/x86-64 architectures.
 * Decomposes address calculation expression trees into [Base + Index * Scale + Disp] SIB operands
 * while tracking folded dead instructions.
 */
class X86AddressingModeMatcher : public MirAddressingModeMatcher
{
  public:
    explicit X86AddressingModeMatcher(MirInstructionSelector *selector = nullptr);
    void setSelector(MirInstructionSelector *selector) { m_selector = selector; }
    void setCurrentInstruction(MirInstruction *inst) { m_currentInst = inst; }

    bool matchAddress(MirBuilderContext *ctx,
                      MirOperand *addrOp,
                      MatchedAddressingMode &outMode) override;

  private:
    bool matchSubtree(MirBuilderContext *ctx,
                      MirRegister *reg,
                      MatchedAddressingMode &mode,
                      size_t depth);

    bool canFold(MirInstruction *def) const;
    MirInstruction *getDef(MirBuilderContext *ctx, MirRegister *reg, MirInstruction *contextInst) const;

    MirInstructionSelector *m_selector{ nullptr };
    MirInstruction *m_currentInst{ nullptr };
};

namespace EzTriple
{
using MatchedAddressingMode = ::MatchedAddressingMode;
using MirAddressingModeMatcher = ::MirAddressingModeMatcher;
using X86AddressingModeMatcher = ::X86AddressingModeMatcher;
} // namespace EzTriple

#endif // EZTRIPLE_MIR_ADDRESSING_MODE_MATCHER_H
