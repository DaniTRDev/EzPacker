#include "DefaultLegalizerActions/LegalizeCallAction.h"

LegalizeCallAction::LegalizeCallAction(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *LegalizeCallAction::getName() { return "LegalizeCallAction"; }

LegalizeActionResult LegalizeCallAction::run(std::pmr::list<MirInstruction *> &instrList,
                                             std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    bool modifiedMir = false;

    MirInstructionBuilder builder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);
    MirOperandBuilder opBuilder(m_ctx);

    /*
     * First operand = return place / destination.
     */
    MirOperand *returnDest = operands[0];
    MirType *retType = returnDest->getMirType();

    MirFunction *func = instr->getOwner()->getOwner();
    CallingConvDesc *cc = func->getCallingConv();
    bool isSretCall = cc && !cc->canReturnInRegs(retType);

    // Every call gets a unique call token to group its argument/return lifecycle.
    MirRegister *callToken = opBuilder.buildVReg(m_ctx->getTypeTable()->getBindingToken());
    MirRegister *sretAddrReg = nullptr;

    if (isSretCall)
    {
        // We need to allocate the data before actually passing it.
        MirType *ptrType = m_ctx->getTypeTable()->getPtr(retType);
        sretAddrReg = opBuilder.buildVReg(ptrType, "sret_alloc_ptr", instr->getSourceRef());

        builder.ALLOC(instr->getSourceRef(), sretAddrReg);
        auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "LegalizeCallAction");
        diag << instr->getSourceRef() << "Moving CALL result value to SRET (indirect) pointer";
        diag.appendNote("Target func: " + func->getName(), func->getSourceRef());
        diag.appendNote("SRET ptr: " + sretAddrReg->getName(), instr->getSourceRef());

        // Since this call uses SRET, the actual destination register receives data indirectly.
        // Convert the CALL node to a tokenized tracking format.
        operands[0] = callToken;
        modifiedMir = true;
    }
    else if (returnDest->isOfType<MirRegister>())
    {
        // Insert POP_RET AFTER THE CALL, structurally tied to this call's token.
        builder.changeInsertionType(InsertionType::InsertAfter);
        builder.POP_RET(instr->getSourceRef(), callToken, returnDest);
        builder.changeInsertionType(InsertionType::InsertBefore);

        // Swap the target return destination inside the CALL with our tracking token.
        operands[0] = callToken;
        modifiedMir = true;
    }
    else
    {
        // For void functions, place the callToken at index 0.
        operands.insert(operands.begin(), callToken);
        modifiedMir = true;
    }

    // Since we've standardized the layout, the callee reference sits at index 1,
    // and the high-level call arguments begin at index 2.
    size_t startingId = 2;

    // Inject the implicit SRET pointer argument if required by the ABI
    if (isSretCall && sretAddrReg)
    {
        builder.PUSH_ARG(instr->getSourceRef(), callToken, sretAddrReg);
        modifiedMir = true;
    }

    // Insert PUSH_ARG instructions BEFORE THE CALL for all user arguments
    for (size_t i = startingId; i < operands.size(); ++i)
    {
        builder.PUSH_ARG(instr->getSourceRef(), callToken, operands[i]);
        modifiedMir = true;
    }

    // Clear the high-level parameter list out of the core CALL instruction.
    // It now only tracks: [callToken, calleeTarget]
    operands.erase(operands.begin() + int64_t(startingId), operands.end());
    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = modifiedMir };
}