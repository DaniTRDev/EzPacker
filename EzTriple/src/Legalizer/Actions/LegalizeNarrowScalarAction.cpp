#include "Legalizer/Actions/LegalizeNarrowScalarAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

#include <functional>
#include <vector>

namespace LegalizeActions
{

/**
 * Breaks a wide operand into numChunks narrow values. Registers are split with an
 * UNMERGE_VALUES instruction, integer constants are sliced directly, and any other operand is
 * simply replicated across the chunks.
 */
static std::vector<MirOperand *>
splitOperand(MirOperand *op,
             size_t numChunks,
             size_t narrowBits,
             MirType *narrowType,
             const std::function<void(MirInstructionOpCode, const std::vector<MirOperand *> &)> &emitInst,
             MirOperandBuilder &ob)
{
    std::vector<MirOperand *> chunks;
    chunks.reserve(numChunks);

    if (op->isOfType<MirRegister>())
    {
        for (size_t k = 0; k < numChunks; ++k)
        {
            chunks.push_back(ob.buildVReg(narrowType));
        }
        chunks.push_back(op);
        emitInst(MirInstructionOpCode::UNMERGE_VALUES, chunks);
        return chunks;
    }

    if (op->isOfType<MirInteger>())
    {
        MirInteger *imm = op->get<MirInteger>();
        if (numChunks == 2)
        {
            FlexInt loVal = imm->getValue().getLowHalf();
            FlexInt hiVal = imm->getValue().getHighHalf();
            chunks.push_back(ob.buildInt(narrowType, loVal));
            chunks.push_back(ob.buildInt(narrowType, hiVal));
            return chunks;
        }

        uint64_t rawVal = imm->getValue().getU64();
        uint64_t mask = (narrowBits >= 64) ? ~0ULL : ((1ULL << narrowBits) - 1ULL);
        for (size_t k = 0; k < numChunks; ++k)
        {
            uint64_t piece = (k * narrowBits < 64) ? ((rawVal >> (k * narrowBits)) & mask) : 0ULL;
            chunks.push_back(ob.buildInt(narrowType, FlexInt(piece, narrowBits)));
        }
        return chunks;
    }

    for (size_t k = 0; k < numChunks; ++k)
    {
        chunks.push_back(op);
    }
    return chunks;
}

/**
 * Rewrites operations on a wide type into a chain of narrower operations. Register and constant
 * operands are split with UNMERGE_VALUES, arithmetic uses explicit carry/borrow chains, and the
 * per-chunk results are recombined with MERGE_VALUES.
 */
LegalizationResult LegalizeNarrowScalar(LegalizeCtx &ctx, size_t operandSlot, MirType *targetType)
{
    if (!ctx.m_ctx || !ctx.m_targetDesc)
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr)
    {
        return LegalizationResult::NotModified;
    }

    auto &operands = instr->getOperands();
    if (operands.empty())
    {
        return LegalizationResult::NotModified;
    }

    MirTypeTable *typeTable = ctx.m_ctx->getTypeTable();
    MirType *narrowType = targetType;

    size_t narrowBits = narrowType->getTotalSizeInBits();
    if (narrowBits == 0)
    {
        return LegalizationResult::NotModified;
    }

    MirOperand *wideOp = nullptr;
    if (operandSlot < operands.size() && operands[operandSlot] && operands[operandSlot]->getMirType() &&
        operands[operandSlot]->getMirType()->getTotalSizeInBits() > narrowBits)
    {
        wideOp = operands[operandSlot];
    }
    else
    {
        for (MirOperand *op : operands)
        {
            if (op && op->getMirType() && op->getMirType()->getTotalSizeInBits() > narrowBits)
            {
                wideOp = op;
                break;
            }
        }
    }

    if (!wideOp || !wideOp->getMirType())
    {
        return LegalizationResult::NotModified;
    }

    size_t origBits = wideOp->getMirType()->getTotalSizeInBits();
    size_t numChunks = origBits / narrowBits;
    if (numChunks < 2)
    {
        numChunks = 2;
    }

    MirType *carryType = typeTable->i1();
    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    EmitOrdered emitInst(ib, instr);

    MirOperand *dstOp = operands[0];
    bool isCompare = (instr->getMetadata().m_category == MirInstructionCategory::MirCat_Compare);

    // Equality comparisons combine per-chunk results with AND (=) or OR (!=) into a single boolean.
    if (isCompare)
    {
        if (operands.size() < 3)
        {
            return LegalizationResult::Failed;
        }

        auto lhsChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        auto rhsChunks = splitOperand(operands[2], numChunks, narrowBits, narrowType, emitInst, ob);

        if (instr->getOpCode() == MirInstructionOpCode::CMP_EQ)
        {
            MirRegister *acc = nullptr;
            for (size_t k = 0; k < numChunks; ++k)
            {
                MirRegister *eqK = ob.buildVReg(carryType);
                emitInst(MirInstructionOpCode::CMP_EQ, { eqK, lhsChunks[k], rhsChunks[k] });
                if (k == 0)
                {
                    acc = eqK;
                }
                else
                {
                    MirRegister *nextAcc = ob.buildVReg(carryType);
                    emitInst(MirInstructionOpCode::AND, { nextAcc, acc, eqK });
                    acc = nextAcc;
                }
            }
            emitInst(MirInstructionOpCode::MOV, { dstOp, acc });
        }
        else if (instr->getOpCode() == MirInstructionOpCode::CMP_NE)
        {
            MirRegister *acc = nullptr;
            for (size_t k = 0; k < numChunks; ++k)
            {
                MirRegister *neK = ob.buildVReg(carryType);
                emitInst(MirInstructionOpCode::CMP_NE, { neK, lhsChunks[k], rhsChunks[k] });
                if (k == 0)
                {
                    acc = neK;
                }
                else
                {
                    MirRegister *nextAcc = ob.buildVReg(carryType);
                    emitInst(MirInstructionOpCode::OR, { nextAcc, acc, neK });
                    acc = nextAcc;
                }
            }
            emitInst(MirInstructionOpCode::MOV, { dstOp, acc });
        }
        else
        {
            return LegalizationResult::Failed;
        }

        ib.erase(instr);
        return LegalizationResult::Legalized;
    }

    std::vector<MirRegister *> dstChunks;
    dstChunks.reserve(numChunks);
    for (size_t k = 0; k < numChunks; ++k)
    {
        dstChunks.push_back(ob.buildVReg(narrowType));
    }

    if (instr->getOpCode() == MirInstructionOpCode::ADD && operands.size() >= 3)
    {
        auto lhsChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        auto rhsChunks = splitOperand(operands[2], numChunks, narrowBits, narrowType, emitInst, ob);

        // Low chunk produces the initial carry, then each UADDE consumes and forwards carry-out.
        MirRegister *carry = ob.buildVReg(carryType);
        emitInst(MirInstructionOpCode::UADDO, { dstChunks[0], carry, lhsChunks[0], rhsChunks[0] });

        for (size_t k = 1; k < numChunks; ++k)
        {
            MirRegister *carryOut = ob.buildVReg(carryType);
            emitInst(MirInstructionOpCode::UADDE, { dstChunks[k], carryOut, lhsChunks[k], rhsChunks[k], carry });
            carry = carryOut;
        }
    }
    else if (instr->getOpCode() == MirInstructionOpCode::SUB && operands.size() >= 3)
    {
        auto lhsChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        auto rhsChunks = splitOperand(operands[2], numChunks, narrowBits, narrowType, emitInst, ob);

        // Low chunk produces the initial borrow, then each USUBE consumes and forwards borrow-out.
        MirRegister *borrow = ob.buildVReg(carryType);
        emitInst(MirInstructionOpCode::USUBO, { dstChunks[0], borrow, lhsChunks[0], rhsChunks[0] });

        for (size_t k = 1; k < numChunks; ++k)
        {
            MirRegister *borrowOut = ob.buildVReg(carryType);
            emitInst(MirInstructionOpCode::USUBE, { dstChunks[k], borrowOut, lhsChunks[k], rhsChunks[k], borrow });
            borrow = borrowOut;
        }
    }
    else if (instr->getOpCode() == MirInstructionOpCode::NEG && operands.size() >= 2)
    {
        auto srcChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        MirInteger *zero = ob.buildInt(narrowType, FlexInt(int64_t(0), narrowBits));

        MirRegister *borrow = ob.buildVReg(carryType);
        emitInst(MirInstructionOpCode::USUBO, { dstChunks[0], borrow, zero, srcChunks[0] });

        for (size_t k = 1; k < numChunks; ++k)
        {
            MirRegister *borrowOut = ob.buildVReg(carryType);
            emitInst(MirInstructionOpCode::USUBE, { dstChunks[k], borrowOut, zero, srcChunks[k], borrow });
            borrow = borrowOut;
        }
    }
    else if ((instr->getOpCode() == MirInstructionOpCode::NOT || instr->getOpCode() == MirInstructionOpCode::MOV) &&
             operands.size() >= 2)
    {
        auto srcChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        for (size_t k = 0; k < numChunks; ++k)
        {
            emitInst(instr->getOpCode(), { dstChunks[k], srcChunks[k] });
        }
    }
    else if (operands.size() == 3)
    {
        auto lhsChunks = splitOperand(operands[1], numChunks, narrowBits, narrowType, emitInst, ob);
        auto rhsChunks = splitOperand(operands[2], numChunks, narrowBits, narrowType, emitInst, ob);

        for (size_t k = 0; k < numChunks; ++k)
        {
            emitInst(instr->getOpCode(), { dstChunks[k], lhsChunks[k], rhsChunks[k] });
        }
    }
    else
    {
        return LegalizationResult::Failed;
    }

    std::vector<MirOperand *> mergeOps;
    mergeOps.reserve(numChunks + 1);
    mergeOps.push_back(dstOp);
    for (size_t k = 0; k < numChunks; ++k)
    {
        mergeOps.push_back(dstChunks[k]);
    }
    emitInst(MirInstructionOpCode::MERGE_VALUES, mergeOps);

    ib.erase(instr);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
