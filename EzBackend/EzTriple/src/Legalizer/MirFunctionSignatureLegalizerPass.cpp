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

    for (MirRegister *param : func->getParameters())
    {
        builder.POP_ARG(param->getSourceRef(), param);
    }

    // Don't clear the function parameters, might be of use for future passes to have a quick way of getting params.
    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = succeeded };
}

void MirFunctionSignatureLegalizerPass::printResult() const {}
