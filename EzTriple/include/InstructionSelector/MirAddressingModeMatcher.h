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

/**
 * Outcome of matching an address operand against a hardware addressing mode.
 * Captures the base/index registers, scale and displacement of an x86 SIB operand together with
 * the address-computation instructions that became dead once the mode was folded.
 */
struct MatchedAddressingMode
{
    MirRegister *m_base{ nullptr };  ///< Base register of the mode, or nullptr when absent.
    int64_t m_disp{ 0 };             ///< Constant byte displacement added to the computed address.
    MirRegister *m_index{ nullptr }; ///< Scaled index register, or nullptr for a simple [base + disp].
    uint8_t m_scale{ 1 };            ///< Index multiplier (1, 2, 4 or 8).
    std::vector<MirInstruction *> m_foldedInstructions; ///< Address computations folded away and eligible for erasure.
};

class MirAddressingModeMatcher
{
  public:
    virtual ~MirAddressingModeMatcher() = default;

    /**
     * Matches an address operand (register or pointer computation tree) into hardware addressing mode.
     */
    virtual bool matchAddress(MirBuilderContext *ctx, MirOperand *addrOp, MatchedAddressingMode &outMode) = 0;
};

/**
 * Concrete addressing mode matcher for x86/x86-64 architectures.
 * Decomposes address calculation expression trees into [Base + Index * Scale + Disp] SIB operands
 * while tracking folded dead instructions.
 */
class X86AddressingModeMatcher : public MirAddressingModeMatcher
{
  public:
    /**
     * Binds the matcher to the instruction selector whose folding decisions it participates in.
     */
    explicit X86AddressingModeMatcher(MirInstructionSelector *selector = nullptr);

    /// Sets the selector used to resolve register definitions while folding.
    void setSelector(MirInstructionSelector *selector) { m_selector = selector; }

    /// Sets the instruction currently being selected, used to stop folds that would cross it.
    void setCurrentInstruction(MirInstruction *inst) { m_currentInst = inst; }

    /**
     * Decomposes addrOp into a base/index/scale/displacement form when it follows x86 SIB rules.
     * @param ctx Active builder context.
     * @param addrOp Address operand (register or pointer computation) to match.
     * @param outMode Receives the matched addressing mode on success.
     * @return True if a usable addressing mode was recognized.
     */
    bool matchAddress(MirBuilderContext *ctx, MirOperand *addrOp, MatchedAddressingMode &outMode) override;

  private:
    /**
     * Recursively walks a pointer computation tree, accumulating base, index, scale and displacement.
     * @param depth Current recursion depth, bounded to avoid unbounded trees.
     */
    bool matchSubtree(MirBuilderContext *ctx, MirRegister *reg, MatchedAddressingMode &mode, size_t depth);

    /// Returns true when def is a side-effect-free address computation safe to fold into an operand.
    bool canFold(MirInstruction *def) const;

    /**
     * Finds the defining instruction feeding reg within the active function, ignoring
     * the instruction that is being folded so a node cannot reference itself.
     */
    MirInstruction *getDef(MirBuilderContext *ctx, MirRegister *reg, MirInstruction *contextInst) const;

    MirInstructionSelector *m_selector{ nullptr }; ///< Selector consulted to locate register definitions.
    MirInstruction *m_currentInst{ nullptr };      ///< Instruction being selected, defining the fold boundary.
};

namespace EzTriple
{
/// Re-exports the global addressing-mode types under the EzTriple namespace.
using MatchedAddressingMode = ::MatchedAddressingMode;
using MirAddressingModeMatcher = ::MirAddressingModeMatcher;
using X86AddressingModeMatcher = ::X86AddressingModeMatcher;
} // namespace EzTriple

#endif // EZTRIPLE_MIR_ADDRESSING_MODE_MATCHER_H
