#include "Legalizer/Actions/LegalizeReturnAction.h"

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

LegalizationResult LegalizeReturn(LegalizeCtx &ctx)
{
    auto it = ctx.m_it;
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();

    MirBlock *owningBlock = instr->getOwner();
    MirFunction *owningFunc = owningBlock ? owningBlock->getOwner() : nullptr;
    CallingConvDesc *cc = owningFunc ? owningFunc->getCallingConv() : nullptr;
    MirInstructionBuilder insertBeforeBuilder(builderCtx, owningBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(builderCtx);

    // Create a unique return tracking token (virtual register)
    MirRegister *retToken = opBuilder.buildVReg(builderCtx->getTypeTable()->__bindToken());

    if (!operands.empty())
    {
        // Original return value being passed out of the function
        MirOperand *returnVal = operands[0];
        MirType *retType = returnVal ? returnVal->getMirType() : nullptr;

        if (cc && retType && !cc->canReturnInRegs(retType) && owningFunc && !owningFunc->getParameters().empty())
        {
            // By convention the sretPtr is stored in the first parameter
            MirRegister *sretPtr = owningFunc->getParameters().front();

            // Emit: STORE sourceRef, memDest, returnVal
            MirOperand *memDest = opBuilder.buildMem(retType, sretPtr, FlexInt(int64_t(0)));
            insertBeforeBuilder.build(MirInstructionOpCode::STORE, instr->getSourceRef(), { memDest, returnVal });

            // Forward sretPtr as returnVal if required
            returnVal = sretPtr;
        }

        // PUSH_RET groups: (retToken, returnVal)
        insertBeforeBuilder.build(MirInstructionOpCode::PUSH_RET, instr->getSourceRef(), { retToken, returnVal });

        // Replace the RET operand with the tracking token
        operands[0] = retToken;
    }
    else
    {
        operands.push_back(retToken);
    }

    return LegalizationResult::Legalized;
}

} // namespace LegalizeActions
