#include "Legalizer/Actions/LegalizeLibcallAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

#include <algorithm>
#include <vector>

namespace LegalizeActions
{

LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, std::string_view libcallSymbol)
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
        LegalizeCtx newCtx(ctx.m_ctx, ctx.m_targetDesc, callIt);
        return LegalizeCall(newCtx);
    }

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
