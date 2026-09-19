#include "InstructionSelector/MirAddressingModeMatcher.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "Operand/MirOperands.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Builder/MirBuilderContext.h"
#include "Block/MirBlock.h"

/// Stores the selector used to query register uses and definitions during folding.
X86AddressingModeMatcher::X86AddressingModeMatcher(MirInstructionSelector *selector) : m_selector(selector) {}

/**
 * Returns true when def is an unselected, single-use computation that can safely be folded into
 * an addressing mode without crossing a memory-writing or side-effecting instruction.
 */
bool X86AddressingModeMatcher::canFold(MirInstruction *def) const
{
    if (!def)
        return false;
    if (def->isSelected() || def->getTargetDesc() != nullptr || def->isErased())
        return false;

    // Check single-use on the definition's destination register
    if (def->getOperandCount() > 0)
    {
        if (auto *dstReg = def->getOpAs<MirRegister>(0))
        {
            bool hasOne = false;
            if (m_selector && m_selector->hasOneUse(dstReg))
            {
                hasOne = true;
            }
            else if (def->getOwner() && def->getOwner()->getOwner())
            {
                auto *func = def->getOwner()->getOwner();
                if (func->getRegisterInfo() && func->getRegisterInfo()->hasOneUse(dstReg->getRegId()))
                {
                    hasOne = true;
                }
            }
            if (!hasOne)
            {
                return false;
            }
        }
    }

    // Check no intervening store between def and current root instruction
    if (m_selector && m_currentInst)
    {
        if (!m_selector->noInterveningStore(def, m_currentInst))
        {
            return false;
        }
    }

    return true;
}

/**
 * Resolves the defining instruction for a virtual register, trying the selector, the supplied
 * context instruction, the current instruction and finally every function in the module.
 */
MirInstruction *
X86AddressingModeMatcher::getDef(MirBuilderContext *ctx, MirRegister *reg, MirInstruction *contextInst) const
{
    if (!reg || !reg->isVirtual())
        return nullptr;

    if (m_selector)
    {
        if (auto *def = m_selector->getDefiningInstruction(ctx, reg))
            return def;
    }

    if (contextInst && contextInst->getOwner() && contextInst->getOwner()->getOwner())
    {
        auto *func = contextInst->getOwner()->getOwner();
        if (func && func->getRegisterInfo())
            return func->getRegisterInfo()->getDef(reg->getRegId());
    }

    if (m_currentInst && m_currentInst->getOwner() && m_currentInst->getOwner()->getOwner())
    {
        auto *func = m_currentInst->getOwner()->getOwner();
        if (func && func->getRegisterInfo())
            return func->getRegisterInfo()->getDef(reg->getRegId());
    }

    if (ctx)
    {
        for (auto func : ctx->getFunctions())
        {
            if (func->getRegisterInfo())
            {
                if (auto *def = func->getRegisterInfo()->getDef(reg->getRegId()))
                    return def;
            }
        }
    }

    return nullptr;
}

/**
 * Recursively folds ADD/SHL address computations feeding reg into mode, accumulating the
 * displacement and recording each folded definition. Depth is capped to keep matching bounded.
 */
bool X86AddressingModeMatcher::matchSubtree(MirBuilderContext *ctx,
                                            MirRegister *reg,
                                            MatchedAddressingMode &mode,
                                            size_t depth)
{
    if (!reg || depth > 8)
        return false;

    MirInstruction *def = getDef(ctx, reg, nullptr);

    if (!def || !canFold(def))
        return false;

    auto op = def->getOpCode();
    if (op == MirInstructionOpCode::ADD)
    {
        // ADD dst, src1, src2
        auto *src1Reg = def->getOpAs<MirRegister>(1);
        auto *src1Imm = def->getOpAs<MirInteger>(1);
        auto *src2Reg = def->getOpAs<MirRegister>(2);
        auto *src2Imm = def->getOpAs<MirInteger>(2);

        // Case 1: ADD reg, imm
        if (src1Reg && src2Imm)
        {
            mode.m_disp += src2Imm->getValue().getI64();
            mode.m_base = src1Reg;
            mode.m_foldedInstructions.push_back(def);
            matchSubtree(ctx, src1Reg, mode, depth + 1);
            return true;
        }
        // Case 2: ADD imm, reg
        if (src1Imm && src2Reg)
        {
            mode.m_disp += src1Imm->getValue().getI64();
            mode.m_base = src2Reg;
            mode.m_foldedInstructions.push_back(def);
            matchSubtree(ctx, src2Reg, mode, depth + 1);
            return true;
        }
        // Case 3: ADD reg1, reg2
        if (src1Reg && src2Reg && mode.m_index == nullptr)
        {
            // Check if src2 is defined by SHL reg, shift
            MirInstruction *def2 = getDef(ctx, src2Reg, def);
            if (def2 && def2->getOpCode() == MirInstructionOpCode::SHL && canFold(def2))
            {
                auto *shlSrc = def2->getOpAs<MirRegister>(1);
                auto *shlAmt = def2->getOpAs<MirInteger>(2);
                if (shlSrc && shlAmt)
                {
                    int64_t shift = shlAmt->getValue().getI64();
                    if (shift >= 0 && shift <= 3)
                    {
                        mode.m_base = src1Reg;
                        mode.m_index = shlSrc;
                        mode.m_scale = static_cast<uint8_t>(1 << shift);
                        mode.m_foldedInstructions.push_back(def2);
                        mode.m_foldedInstructions.push_back(def);
                        matchSubtree(ctx, src1Reg, mode, depth + 1);
                        return true;
                    }
                }
            }

            // Commutative check: if src1 is defined by SHL reg, shift
            MirInstruction *def1 = getDef(ctx, src1Reg, def);
            if (def1 && def1->getOpCode() == MirInstructionOpCode::SHL && canFold(def1))
            {
                auto *shlSrc = def1->getOpAs<MirRegister>(1);
                auto *shlAmt = def1->getOpAs<MirInteger>(2);
                if (shlSrc && shlAmt)
                {
                    int64_t shift = shlAmt->getValue().getI64();
                    if (shift >= 0 && shift <= 3)
                    {
                        mode.m_base = src2Reg;
                        mode.m_index = shlSrc;
                        mode.m_scale = static_cast<uint8_t>(1 << shift);
                        mode.m_foldedInstructions.push_back(def1);
                        mode.m_foldedInstructions.push_back(def);
                        matchSubtree(ctx, src2Reg, mode, depth + 1);
                        return true;
                    }
                }
            }

            // Neither is SHL: base = src1, index = src2, scale = 1
            mode.m_base = src1Reg;
            mode.m_index = src2Reg;
            mode.m_scale = 1;
            mode.m_foldedInstructions.push_back(def);
            matchSubtree(ctx, src1Reg, mode, depth + 1);
            return true;
        }
    }
    else if (op == MirInstructionOpCode::SHL && mode.m_index == nullptr)
    {
        // SHL dst, src1, shift
        auto *shlSrc = def->getOpAs<MirRegister>(1);
        auto *shlAmt = def->getOpAs<MirInteger>(2);
        if (shlSrc && shlAmt)
        {
            int64_t shift = shlAmt->getValue().getI64();
            if (shift >= 0 && shift <= 3)
            {
                mode.m_index = shlSrc;
                mode.m_scale = static_cast<uint8_t>(1 << shift);
                if (mode.m_base == reg)
                {
                    mode.m_base = nullptr;
                }
                mode.m_foldedInstructions.push_back(def);
                return true;
            }
        }
    }

    return false;
}

/**
 * Entry point: resets outMode and fills it from an existing memory operand, or decomposes a
 * register/pointer computation tree, or treats a bare immediate as a displacement.
 */
bool X86AddressingModeMatcher::matchAddress(MirBuilderContext *ctx, MirOperand *addrOp, MatchedAddressingMode &outMode)
{
    if (!addrOp)
        return false;

    outMode.m_base = nullptr;
    outMode.m_index = nullptr;
    outMode.m_scale = 1;
    outMode.m_disp = 0;
    outMode.m_foldedInstructions.clear();

    if (auto *mem = addrOp->get<MirMemory>())
    {
        outMode.m_base = mem->getBase();
        outMode.m_index = mem->getIndex();
        outMode.m_scale = mem->getScale();
        outMode.m_disp = mem->getDisplacement() ? mem->getDisplacement()->getValue().getI64() : 0;

        if (outMode.m_base && outMode.m_index == nullptr)
        {
            matchSubtree(ctx, outMode.m_base, outMode, 0);
        }
        return true;
    }

    if (auto *reg = addrOp->get<MirRegister>())
    {
        outMode.m_base = reg;
        matchSubtree(ctx, reg, outMode, 0);
        return true;
    }

    if (auto *imm = addrOp->get<MirInteger>())
    {
        outMode.m_disp = imm->getValue().getI64();
        return true;
    }

    return false;
}
