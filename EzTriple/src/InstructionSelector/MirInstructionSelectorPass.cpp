#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "Descriptors/TargetDesc.h"
#include "Function/MirFunction.h"
#include "InstructionSelector/MirInstructionSelector.h"

MirInstructionSelectorPass::MirInstructionSelectorPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

const char *MirInstructionSelectorPass::getName() const
{
    return "MirInstructionSelectorPass";
}

MirPassIterationPlace MirInstructionSelectorPass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

MirPassResult MirInstructionSelectorPass::run(IntrusiveLinkedList<MirFunction> &funcList,
                                              IntrusiveLinkedList<MirFunction>::iterator it,
                                              MirPassManager *passManager)
{
    MirFunction *func = *it;
    if (!func)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    bool modified = false;
    if (m_targetDesc && m_targetDesc->getInstructionSelector())
    {
        bool ok = m_targetDesc->getInstructionSelector()->selectFunction(m_ctx, func);
        if (!ok)
        {
            return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = false };
        }
        modified = true;
    }

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = true };
}
