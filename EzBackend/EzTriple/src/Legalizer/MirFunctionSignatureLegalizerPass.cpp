#include "Legalizer/MirFunctionSignatureLegalizerPass.h"

MirFunctionSignatureLegalizerPass::MirFunctionSignatureLegalizerPass(MirBuilderContext *ctx, MirLegalizer *legalizer) :
    m_ctx(ctx), m_legalizer(legalizer)
{
}

const char *MirFunctionSignatureLegalizerPass::getName() const { return "MirFunctionSignatureLegalizerPass"; }

MirPassIterationPlace MirFunctionSignatureLegalizerPass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

MirPassResult MirFunctionSignatureLegalizerPass::run(std::pmr::list<MirFunction *> &funcList,
                                                     std::pmr::list<struct MirFunction *>::iterator it,
                                                     struct MirPassManager *passManager)
{
    bool modified = false, succeeded = true;
    MirFunction *func = *it;
    MirBlock *entryPoint = func->getEntryPoint();

    MirInstructionBuilder builder(m_ctx,
                                  entryPoint,
                                  InsertionType::InsertBefore,
                                  entryPoint->getInstructions().begin());
    MirOperandBuilder opBuilder(m_ctx);

    CallingConvDesc *cc = func->getCallingConv();
    if (!cc->canReturnInRegs(func->getReturnType()))
    {
        // Allocate a hidden, implicit pointer register. This will store the memory address supplied by the caller where
        // we write the output struct.
        MirType *ptrType = m_ctx->getTypeTable()->getPtr(func->getReturnType());
        MirRegister *sretPtrParam = opBuilder.buildVReg(ptrType, "sret_ptr", func->getSourceRef());

        // Prepend this implicit argument directly to the front of the declaration parameter slice
        func->getParameters().push_front(sretPtrParam);
        modified = true;
    }

    for (MirRegister *param : func->getParameters())
    {
        builder.POP_ARG(param->getSourceRef(), param);
    }

    // Don't clear the function parameters, might be of use for future passes to have a quick way of getting params.
    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = succeeded };
}

void MirFunctionSignatureLegalizerPass::printResult() const {}
