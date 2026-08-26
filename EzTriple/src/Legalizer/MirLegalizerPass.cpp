#include "Legalizer/MirLegalizerPass.h"
#include "Descriptors/TargetDesc.h"
#include "Function/MirFunction.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizer.h"

MirLegalizerPass::MirLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

const char *MirLegalizerPass::getName() const
{
    return "MirLegalizerPass";
}

MirPassIterationPlace MirLegalizerPass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

MirPassResult MirLegalizerPass::run(IntrusiveLinkedList<MirFunction> &funcList,
                                    IntrusiveLinkedList<MirFunction>::iterator it,
                                    MirPassManager *passManager)
{
    (void)funcList;
    (void)passManager;

    MirFunction *func = *it;
    if (!func)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    bool modified = false;

    // 1. Legalize Function Signature (SRET param, POP_ARG, END_ARG)
    MirFunctionSignatureLegalizerPass sigPass(m_ctx, m_targetDesc);
    auto sigRes = sigPass.run(funcList, it, passManager);
    if (!sigRes.m_succeeded)
    {
        return sigRes;
    }
    if (sigRes.m_modifiedMir)
    {
        modified = true;
    }

    // 2. Legalize generic operations, calls, and returns via target legalizer
    if (m_targetDesc && m_targetDesc->getLegalizer())
    {
        bool legOk = m_targetDesc->getLegalizer()->legalizeFunction(func);
        if (!legOk)
        {
            return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = false };
        }
        modified = true;
    }

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = true };
}
