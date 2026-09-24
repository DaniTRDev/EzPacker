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

/**
 * Rewrites a high-level CALL into a token-bound sequence: optional SRET ALLOC/PUSH_ARG, one
 * PUSH_ARG per argument, the CALL itself, and a trailing POP_RET that captures the result.
 */
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

    MirInstructionBuilder beforeBuilder(builderCtx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(builderCtx);

    EmitOrdered emitBefore(beforeBuilder, instr);

    MirOperand *returnDest = nullptr;
    MirOperand *callee = nullptr;
    size_t startingId = 0;

    if (operands[0] && operands[0]->isOfType<MirRegister>())
    {
        MirRegister *reg = operands[0]->get<MirRegister>();
        if (reg->getMirType() && reg->getMirType()->getKind() == MirTypeKind::BindingToken)
        {
            // Already legalized with a call token
            return LegalizationResult::NotModified;
        }

        returnDest = operands[0];
        callee = (operands.size() > 1) ? operands[1] : nullptr;
        startingId = 2;
    }
    else
    {
        returnDest = nullptr;
        callee = operands[0];
        startingId = 1;
    }

    if (!callee)
    {
        return LegalizationResult::Failed;
    }

    MirType *retType = returnDest ? returnDest->getMirType() : nullptr;
    MirFunction *func = instr->getOwner()->getOwner();
    CallingConvDesc *cc = func ? func->getCallingConv() : nullptr;
    bool isSretCall = cc && retType && !cc->canReturnInRegs(retType);

    MirRegister *callToken = opBuilder.buildVReg(builderCtx->getTypeTable()->__bindToken());
    MirRegister *sretAddrReg = nullptr;

    if (isSretCall)
    {
        MirType *ptrType = builderCtx->getTypeTable()->getPtr(retType);
        sretAddrReg = opBuilder.buildVReg(ptrType, "sRetPtr", instr->getSourceRef());
        emitBefore(MirInstructionOpCode::ALLOC, { sretAddrReg });
        emitBefore(MirInstructionOpCode::PUSH_ARG, { callToken, sretAddrReg });
    }

    for (size_t i = startingId; i < operands.size(); ++i)
    {
        if (operands[i])
        {
            emitBefore(MirInstructionOpCode::PUSH_ARG, { callToken, operands[i] });
        }
    }

    if (!isSretCall && returnDest && returnDest->isOfType<MirRegister>())
    {
        MirInstructionBuilder afterBuilder(builderCtx, instr->getOwner(), InsertionType::InsertAfter, it);
        afterBuilder.build(MirInstructionOpCode::POP_RET, instr->getSourceRef(), { callToken, returnDest });
    }

    beforeBuilder.clearOperands(instr).addOperand(instr, callToken).addOperand(instr, callee);
    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
