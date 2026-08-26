#include "Legalizer/Actions/LegalizeLibcallAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

namespace LegalizeActions
{

LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, std::string_view libcallSymbol)
{
    if (!ctx.m_ctx || libcallSymbol.empty())
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr || instr->getOperands().empty())
    {
        return LegalizationResult::NotModified;
    }

    MirInstructionBuilder ib(ctx.m_ctx, instr->getOwner(), InsertionType::InsertBefore, ctx.m_it);
    MirOperandBuilder ob(ctx.m_ctx);

    MirOperand *dst = instr->getOperand(0);
    std::pmr::string symStr(libcallSymbol, ctx.m_ctx->getGlobalAllocator());
    MirRuntimeSymbol *calleeRef = ob.buildRtSymbol(symStr);

    std::vector<MirOperand *> callOps;
    callOps.push_back(dst);
    callOps.push_back(calleeRef);
    for (size_t i = 1; i < instr->getOperands().size(); ++i)
    {
        callOps.push_back(instr->getOperand(i));
    }

    MirInstruction *callInst = ib.build(MirInstructionOpCode::CALL, instr->getSourceRef(), callOps);
    instr->getOwner()->getInstructions().erase(ctx.m_it);

    // Run LegalizeCall on the newly formed CALL instruction
    auto callIt = std::find(instr->getOwner()->getInstructions().begin(),
                            instr->getOwner()->getInstructions().end(),
                            callInst);
    if (callIt != instr->getOwner()->getInstructions().end())
    {
        LegalizeCtx newCtx(ctx.m_ctx, ctx.m_targetDesc, callIt);
        return LegalizeCall(newCtx);
    }

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
