#include "Legalizer/Actions/LegalizeCallAction.h"

#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

namespace LegalizeActions
{

LegalizationResult LegalizeCall(LegalizeCtx &ctx)
{
    auto it = ctx.m_it;
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();

    if (operands.empty())
    {
        return LegalizationResult::NotModified;
    }

    MirInstructionBuilder builder(builderCtx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(builderCtx);

    /*
     * First operand = return place / destination.
     */
    MirOperand *returnDest = operands[0];
    MirType *retType = returnDest ? returnDest->getMirType() : nullptr;

    MirFunction *func = instr->getOwner()->getOwner();
    CallingConvDesc *cc = func ? func->getCallingConv() : nullptr;
    bool isSretCall = cc && retType && !cc->canReturnInRegs(retType);

    // Every call gets a unique call token to group its argument/return lifecycle.
    MirRegister *callToken = opBuilder.buildVReg(builderCtx->getTypeTable()->__bindToken());
    MirRegister *sretAddrReg = nullptr;

    if (isSretCall)
    {
        // We need to allocate the data before actually passing it.
        MirType *ptrType = builderCtx->getTypeTable()->getPtr(retType);
        sretAddrReg = opBuilder.buildVReg(ptrType, "sRetPtr", instr->getSourceRef());

        builder.ALLOC(instr->getSourceRef(), sretAddrReg);

        // Since this call uses SRET, the actual destination register receives data indirectly.
        // Convert the CALL node to a tokenized tracking format.
        operands[0] = callToken;
    }
    else if (returnDest && returnDest->isOfType<MirRegister>())
    {
        // Insert POP_RET AFTER THE CALL, structurally tied to this call's token.
        builder.changeInsertionType(InsertionType::InsertAfter);
        builder.POP_RET(instr->getSourceRef(), callToken, returnDest);
        builder.changeInsertionType(InsertionType::InsertBefore);

        // Swap the target return destination inside the CALL with our tracking token.
        operands[0] = callToken;
    }
    else
    {
        // For void functions, place the callToken at index 0.
        operands.insert(operands.begin(), callToken);
    }

    // Standardized layout: index 0 is callToken, index 1 is callee, index 2+ are args.
    size_t startingId = 2;

    // Inject the implicit SRET pointer argument if required by the ABI
    if (isSretCall && sretAddrReg)
    {
        builder.PUSH_ARG(instr->getSourceRef(), callToken, sretAddrReg);
    }

    // Insert PUSH_ARG instructions BEFORE THE CALL for all user arguments
    for (size_t i = startingId; i < operands.size(); ++i)
    {
        builder.PUSH_ARG(instr->getSourceRef(), callToken, operands[i]);
    }

    // Clear the high-level parameter list out of the core CALL instruction.
    // It now only tracks: [callToken, calleeTarget]
    if (operands.size() > startingId)
    {
        operands.erase(operands.begin() + int64_t(startingId), operands.end());
    }

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
