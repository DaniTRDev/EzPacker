#include "Legalizer/Actions/LegalizeReturnAction.h"

namespace LegalizeActions
{
LegalizationResult LegalizeReturn(LegalizeCtx &ctx)
{
    auto it = ctx.m_it;
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();

    MirBlock *owningBlock = instr->getOwner();
    MirFunction *owningFunc = owningBlock->getOwner();
    CallingConvDesc *cc = owningFunc->getCallingConv();
    MirInstructionBuilder insertBeforeBuilder(builderCtx, owningBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(builderCtx);

    // Create a unique return tracking token (virtual register)
    MirRegister *retToken = opBuilder.buildVReg(builderCtx->getTypeTable()->getBindingToken());

    // For non-void functions.
    if (!operands.empty())
    {
        // Original return value being passed out of the function
        MirOperand *returnVal = operands[0];

        if (!cc->canReturnInRegs(returnVal->getMirType()))
        {
            // By convention the sretPtr is stored in the first parameter, ALWAYS.
            MirRegister *sretPtr = owningFunc->getParameters().front();

            auto diag = builderCtx->getDiagCollector()->builder(Diag_Trace, "LegalizeReturnAction");
            diag << returnVal->getSourceRef() << "Function's returns expects an SRET, moving it";
            diag.appendNote(MirPrinter::printToString(sretPtr).c_str(), sretPtr->getSourceRef());
            diag.appendNote(MirPrinter::printToString(owningFunc, MirPrinterDetail::General).c_str(),
                            owningFunc->getSourceRef());

            // This memory dest is going to automatically be promoted/expanded during legalization.
            MirOperand *memDest = opBuilder.buildMem(returnVal->getMirType(), sretPtr, FlexInt(int64_t(0)));

            // Emit: STORE sourceRef, memDest, returnVal
            insertBeforeBuilder.STORE(instr->getSourceRef(), memDest, returnVal);

            /**
             * Made the legalizer also emit PUSH_RET type* %returnVal because in some ABIs, the SRET pointer might also
             * be returned in a specific place. It's the job of the ABI lowerer to get rid, or not, of this.
             */
            returnVal = sretPtr;
        }

        // PUSH_RET now explicitly groups: (sourceRef, retToken, returnVal)
        insertBeforeBuilder.PUSH_RET(instr->getSourceRef(), retToken, returnVal);

        // Instead of clearing the operands entirely, modify the RET instruction
        // to store the tracking token. A fully legalized RET now looks like: [retToken]
        operands[0] = retToken;
    }
    else
    {
        operands.push_back(retToken);
    }

    return LegalizationResult::Legalized;
}
}; // namespace LegalizeActions
