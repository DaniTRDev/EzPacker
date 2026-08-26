#include "Legalizer/Actions/LegalizeCustomAction.h"

#include "Descriptors/TargetDesc.h"
#include "Instruction/MirInstruction.h"
#include "Legalizer/MirExpansionRuleRegistry.h"

namespace LegalizeActions
{

LegalizationResult LegalizeCustom(LegalizeCtx &ctx)
{
    if (!ctx.m_ctx || !ctx.m_targetDesc)
    {
        return LegalizationResult::Failed;
    }

    MirInstruction *instr = *ctx.m_it;
    if (!instr)
    {
        return LegalizationResult::NotModified;
    }

    auto *expRules = ctx.m_targetDesc->getExpansionRegistry();
    if (expRules && expRules->tryExpand(ctx.m_ctx, instr))
    {
        return LegalizationResult::Legalized;
    }

    return LegalizationResult::Failed;
}

} // namespace LegalizeActions
