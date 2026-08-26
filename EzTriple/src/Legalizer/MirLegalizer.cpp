#include "Legalizer/MirLegalizer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/MirExpansionRuleRegistry.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

bool MirLegalizer::legalizeFunction(MirFunction *func)
{
    if (!func) return false;

    bool allSucceeded = true;
    for (MirBlock *block : func->getBlocks())
    {
        if (!legalizeBlock(block))
        {
            allSucceeded = false;
        }
    }
    return allSucceeded;
}

bool MirLegalizer::legalizeBlock(MirBlock *block)
{
    if (!block) return false;

    auto &instList = block->getInstructions();
    auto it = instList.begin();

    while (it != instList.end())
    {
        auto curIt = it;
        ++it;

        LegalizationResult res = legalizeInstruction(curIt, block);
        if (res == LegalizationResult::Failed)
        {
            return false;
        }
    }

    return true;
}

LegalizationResult MirLegalizer::legalizeInstruction(IntrusiveLinkedList<MirInstruction>::iterator it, MirBlock *block)
{
    (void)block;
    if (!m_ctx) return LegalizationResult::Failed;

    MirInstruction *inst = *it;
    if (!inst) return LegalizationResult::NotModified;

    LegalizeCtx ctx(m_ctx, m_targetDesc, it);

    if (inst->getFlags() & MirInstructionFlags::IsCall)
    {
        return LegalizeActions::LegalizeCall(ctx);
    }
    if (inst->getFlags() & MirInstructionFlags::IsReturn)
    {
        return LegalizeActions::LegalizeReturn(ctx);
    }

    // Attempt custom expansion rewrite rules if available
    if (m_targetDesc)
    {
        auto *expRules = m_targetDesc->getExpansionRegistry();
        if (expRules && expRules->tryExpand(m_ctx, inst))
        {
            return LegalizationResult::Legalized;
        }
    }

    return LegalizationResult::NotModified;
}
