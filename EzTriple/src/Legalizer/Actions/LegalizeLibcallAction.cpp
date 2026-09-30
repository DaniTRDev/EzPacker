#include "Legalizer/Actions/LegalizeLibcallAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Libcall/LibcallKind.h"
#include "Libcall/TargetLibcallRegistry.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

#include <algorithm>
#include <vector>

namespace LegalizeActions
{

/**
 * Replaces the current instruction with a CALL to libcallSymbol that forwards the original
 * operands, then reuses LegalizeCall to apply the target calling convention.
 */
LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, std::string_view libcallSymbol, CallingConvDesc *overrideCC)
{
    if (!ctx.m_ctx || libcallSymbol.empty())
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr)
    {
        return LegalizationResult::NotModified;
    }

    MirBlock *ownerBlock = instr->getOwner();
    MirInstructionBuilder ib(ctx.m_ctx, ownerBlock, InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    std::pmr::string symStr(libcallSymbol, ctx.m_ctx->getGlobalAllocator());
    MirRuntimeSymbol *calleeRef = ob.buildRtSymbol(symStr);

    std::vector<MirOperand *> callOps;
    callOps.reserve(instr->getOperandCount() + 1);
    // A write-only operand 0 is the original destination; the libcall result must be returned into it.
    bool hasDst = (instr->hasOperands() && (instr->getOperandFlag(0) & MirOperandFlag::Write));

    if (hasDst)
    {
        callOps.push_back(instr->getOperand(0));
        callOps.push_back(calleeRef);
        for (size_t i = 1; i < instr->getOperandCount(); ++i)
        {
            callOps.push_back(instr->getOperand(i));
        }
    }
    else
    {
        callOps.push_back(calleeRef);
        for (size_t i = 0; i < instr->getOperandCount(); ++i)
        {
            callOps.push_back(instr->getOperand(i));
        }
    }

    MirInstruction *callInst = ib.build(MirInstructionOpCode::CALL, instr->getSourceRef(), callOps);
    ib.erase(instr);

    // Run LegalizeCall on the newly formed CALL instruction
    auto callIt = std::find(ownerBlock->getInstructions().begin(), ownerBlock->getInstructions().end(), callInst);
    if (callIt != ownerBlock->getInstructions().end())
    {
        MirFunction *func = ownerBlock->getOwner();
        CallingConvDesc *savedCC = func ? func->getCallingConv() : nullptr;

        if (overrideCC && func && overrideCC != savedCC)
        {
            func->setCallingConv(overrideCC);
        }

        LegalizeCtx newCtx(ctx.m_ctx, ctx.m_targetDesc, callIt);
        auto result = LegalizeCall(newCtx);

        if (overrideCC && func && overrideCC != savedCC)
        {
            func->setCallingConv(savedCC);
        }

        return result;
    }

    return LegalizationResult::Legalized;
}

/**
 * Resolves the LibcallKind through the target's TargetLibcallRegistry (or defaults),
 * checks availability, determines the appropriate calling convention, and lowers the call.
 */
LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, LibcallKind kind)
{
    if (!ctx.m_ctx)
    {
        return LegalizationResult::Failed;
    }

    std::string_view sym;
    CallingConvDesc *cc = nullptr;

    if (ctx.m_targetDesc)
    {
        if (auto *reg = ctx.m_targetDesc->getLibcallRegistry())
        {
            if (!reg->isAvailable(kind))
            {
                return LegalizationResult::Failed;
            }
            sym = reg->getLibcallName(kind);
            cc = reg->getCallingConvention(kind);
        }
        else
        {
            sym = ctx.m_targetDesc->getLibcallStr(kind);
        }
    }

    if (sym.empty())
    {
        sym = getDefaultLibcallName(kind);
    }

    return LegalizeLibcall(ctx, sym, cc);
}

} // namespace LegalizeActions
