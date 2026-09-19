#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "Descriptors/TargetDesc.h"
#include "Function/MirFunction.h"
#include "InstructionSelector/MirInstructionSelector.h"

/**
 * Stores the shared context and target descriptor used when running the pass.
 */
MirInstructionSelectorPass::MirInstructionSelectorPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

/// Returns the diagnostic name of this pass.
const char *MirInstructionSelectorPass::getName() const { return "MirInstructionSelectorPass"; }

/// Runs once per function rather than once per module.
MirPassIterationPlace MirInstructionSelectorPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

/**
 * Selects target instructions for the function referenced by it.
 * Fails without modification when the function is null or the target has no selector.
 */
MirPassResult MirInstructionSelectorPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
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
        // Delegate the whole function to the target selector; it handles all blocks.
        bool ok = m_targetDesc->getInstructionSelector()->selectFunction(m_ctx, func);
        if (!ok)
        {
            return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = false };
        }
        modified = true;
    }

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = true };
}
