#include "TargetLegalizer/StandardLegalizers/ExpandTypeLegalizer.h"
#include "TargetDesc.h"
#include "Emitter/MirEmitter.h"
#include <vector>
#include <array>
#include <format>

static std::array<MirRegister *, 2> expandRegister(LegalizerContext *ctx, MirRegister *reg, MirType *halvedType)
{
    if (ctx->isRegisterExpanded(reg))
        return ctx->getExpandedRegister(reg);

    MirEmitter *emitter = ctx->getEmitter();

    // Allocate the two new halved registers directly into the pool
    MirRegister *lowReg = emitter->createVirtualRegister(halvedType);
    MirRegister *highReg = emitter->createVirtualRegister(halvedType);

    ctx->addExpandedRegister(reg, { lowReg, highReg });
    return { lowReg, highReg };
}

static std::array<MirInteger *, 2> expandInteger(LegalizerContext *ctx, MirInteger *imm, MirType *halvedType)
{
    uint64_t val = static_cast<uint64_t>(imm->getValue());
    size_t legalSizeInBytes = halvedType->getTotalSizeInBytes();

    // Create a mask for the legal size (e.g., 0xFFFFFFFF for 32-bit)
    uint64_t mask = (1ULL << (legalSizeInBytes * 8)) - 1;

    // Use the Context Factory to allocate the halves safely in the pool
    MirInteger *lo = ctx->getEmitter()->createImmediateInteger(halvedType, static_cast<int64_t>(val & mask));
    MirInteger *hi =
            ctx->getEmitter()->createImmediateInteger(halvedType,
                                                      static_cast<int64_t>((val >> (legalSizeInBytes * 8)) & mask));

    return { lo, hi };
}

static std::array<MirOperand *, 2> getExpandedOperand(LegalizerContext *ctx, MirOperand *operand, MirType *halvedType)
{
    std::array<MirOperand *, 2> result = { nullptr, nullptr };

    if (operand->isOfType<MirRegister>())
    {
        auto regs = expandRegister(ctx, operand->get<MirRegister>(), halvedType);
        result[0] = regs[0];
        result[1] = regs[1];
    }
    else if (operand->isOfType<MirInteger>())
    {
        auto imms = expandInteger(ctx, operand->get<MirInteger>(), halvedType);
        result[0] = imms[0];
        result[1] = imms[1];
    }
    else if (operand->isOfType<MirDouble>())
    {
        // TODO: Add after adding support to FPU.
    }

    return result;
}

bool StandardLegalizers::expandTypeLegalizer(LegalizerContext *ctx,
                                             TypedPoolLinkedList<class MirInstruction> *instrList,
                                             TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    MirEmitter *emitter = ctx->getEmitter();
    MirEmitterContext *emitterCtx = emitter->getContext();
    MirInstruction *instr = *it;
    MirBlock *currentBlock = emitterCtx->getCurrentBoundBlock();

    const auto &linearEquiv = instr->getMetadata().m_linearEquivalent;
    if (linearEquiv.m_high == MirInstructionOpCode::INVALID || linearEquiv.m_low == MirInstructionOpCode::INVALID)
    {
        emitterCtx->emitError(
                ErrorSeverity::Fatal,
                std::format("Internal Compiler Error: No linear equivalent for opcode {}", instr->getMetadata().m_name),
                "StandardLegalizers::expandTypeLegalizer");
        return false;
    }
    
    size_t originalSize = instr->getOperands()->get<MirOperand>(0)->getSizeInBytes();
    MirType *halvedType = emitterCtx->getIntegerTypeBySize(originalSize / 2);

    std::vector<MirOperand *> lowOperands;
    std::vector<MirOperand *> highOperands;

    TypedPoolLinkedList<MirOperand> *operands = instr->getOperands();
    for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
    {
        MirOperand *op = *opIt;
        auto expandedPair = getExpandedOperand(ctx, op, halvedType);

        lowOperands.push_back(expandedPair[0]);
        highOperands.push_back(expandedPair[1]);
    }

    // Lock insertion to happen BEFORE the current illegal instruction
    emitterCtx->setInsertPoint(currentBlock, it);

    // Emit LOW instruction (e.g., ADD)
    // Because we use InsertBefore, it goes immediately before the old instruction.
    MirInstruction *lowInstr = emitter->emit(linearEquiv.m_low);
    for (MirOperand *op : lowOperands)
    {
        emitter->emitOperandToInstruction(lowInstr, op);
    }

    // Emit HIGH instruction (e.g., ADC)
    // It still inserts before the original instruction, meaning it naturally places itself right after the LOW
    // instruction we just emitted.
    MirInstruction *highInstr = emitter->emit(linearEquiv.m_high);
    for (MirOperand *op : highOperands)
    {
        emitter->emitOperandToInstruction(highInstr, op);
    }

    // Reset instruction insertion point so it defaults back to append mode
    emitterCtx->setInsertPoint(currentBlock);

    // Completely remove the original illegal instruction from the linked list
    emitterCtx->getInstructionPool()->removeFromList(instrList, it);

    return true;
}