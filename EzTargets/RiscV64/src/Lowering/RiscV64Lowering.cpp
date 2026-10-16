#include "RiscV64Lowering.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeLibcallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include <bit>
#include <cstdint>

/// Entry point registered as the RISC-V 64 CALL/lower handler; dispatches to shared call legalizer.
LegalizationResult RiscV64CallLowering(LegalizeCtx &ctx)
{
    return LegalizeActions::LegalizeCall(ctx);
}

/// Entry point registered as the RISC-V 64 RET/lower handler; dispatches to shared return legalizer.
LegalizationResult RiscV64ReturnLowering(LegalizeCtx &ctx)
{
    return LegalizeActions::LegalizeReturn(ctx);
}

/// Entry point registered as the RISC-V 64 THROW/lower handler; lowers to runtime __ez_throw call.
LegalizationResult RiscV64ThrowLowering(LegalizeCtx &ctx)
{
    if (!ctx.m_ctx)
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr)
    {
        return LegalizationResult::NotModified;
    }

    MirOperandBuilder ob(ctx.m_ctx);
    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);

    if (instr->getOperandCount() == 1)
    {
        std::pmr::string rttiSym("__ez_default_rtti", ctx.m_ctx->getGlobalAllocator());
        MirRuntimeSymbol *rttiOp = ob.buildRtSymbol(rttiSym);
        ib.addOperand(instr, rttiOp);
    }
    else if (instr->getOperandCount() == 0)
    {
        std::pmr::string payloadSym("__ez_default_payload", ctx.m_ctx->getGlobalAllocator());
        MirRuntimeSymbol *payloadOp = ob.buildRtSymbol(payloadSym);
        std::pmr::string rttiSym("__ez_default_rtti", ctx.m_ctx->getGlobalAllocator());
        MirRuntimeSymbol *rttiOp = ob.buildRtSymbol(rttiSym);
        ib.addOperand(instr, payloadOp);
        ib.addOperand(instr, rttiOp);
    }

    return LegalizeActions::LegalizeLibcall(ctx, "__ez_throw");
}

/// Entry point registered as the RISC-V 64 CATCH/lower handler; lowers to runtime __ez_get_current_exception call.
LegalizationResult RiscV64CatchLowering(LegalizeCtx &ctx)
{
    return LegalizeActions::LegalizeLibcall(ctx, "__ez_get_current_exception");
}

/// Predicate used by legalization rules: true when val is a positive power of two.
bool isPowTwo(int64_t val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

/// Predicate used by legalization rules: true when val is a strictly positive constant.
bool isPositiveConst(int64_t val)
{
    return val > 0;
}

/// Returns the shift amount for a positive power of two (floor(log2)), or 0 otherwise.
int64_t log2Pow2(int64_t val)
{
    if (val <= 0)
        return 0;
    return std::countr_zero(static_cast<uint64_t>(val));
}

/// Returns val - 1, used by rule transforms that match a power-of-two-minus-one pattern.
int64_t sub1(int64_t val)
{
    return val - 1;
}
