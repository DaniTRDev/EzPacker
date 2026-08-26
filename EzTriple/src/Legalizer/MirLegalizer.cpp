#include "Legalizer/MirLegalizer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Legalizer/Actions/LegalizeBitcastAction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeCustomAction.h"
#include "Legalizer/Actions/LegalizeLibcallAction.h"
#include "Legalizer/Actions/LegalizeNarrowScalarAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/Actions/LegalizeWidenScalarAction.h"
#include "Legalizer/MirExpansionRuleRegistry.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

MirLegalizer::MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc) : m_ctx(ctx), m_targetDesc(targetDesc) {}

bool MirLegalizer::legalizeFunction(MirFunction *func)
{
    if (!func)
        return false;

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
    if (!block)
        return false;

    constexpr size_t MaxPasses = 32;
    size_t passCount = 0;
    bool changed = true;

    while (changed && passCount < MaxPasses)
    {
        changed = false;
        passCount++;

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
            if (res == LegalizationResult::Legalized)
            {
                changed = true;
                // Restart iteration on this block to ensure newly introduced instructions are legalized
                break;
            }
        }
    }

    return true;
}

LegalizationResult MirLegalizer::legalizeInstruction(IntrusiveLinkedList<MirInstruction>::iterator it, MirBlock *block)
{
    if (!m_ctx)
        return LegalizationResult::Failed;

    MirInstruction *inst = *it;
    if (!inst)
        return LegalizationResult::NotModified;

    LegalizeCtx ctx(m_ctx, m_targetDesc, it);

    // 1. High-level calling convention instructions
    if (inst->getFlags() & MirInstructionFlags::IsCall)
    {
        return LegalizeActions::LegalizeCall(ctx);
    }
    if (inst->getFlags() & MirInstructionFlags::IsReturn)
    {
        return LegalizeActions::LegalizeReturn(ctx);
    }

    // 2. Custom expansion rewrite rules (.lrd)
    if (m_targetDesc)
    {
        auto *expRules = m_targetDesc->getExpansionRegistry();
        if (expRules && expRules->tryExpand(m_ctx, inst))
        {
            return LegalizationResult::Legalized;
        }
    }

    // 3. Action query per operand slot
    for (size_t slot = 0; slot < inst->getOperandCount(); ++slot)
    {
        LegalizeAction action = getTargetLegalizeAction(inst, slot);
        MirType *targetType = getTargetLegalType(inst, slot);

        if (action == LegalizeAction::Unsupported)
        {
            m_ctx->getDiagCollector()->error("MirLegalizer",
                                             "Invalid target legalize action for instruction's operand slot: {}",
                                             slot)
                    << inst->getSourceRef();
            return LegalizationResult::Failed;
        }

        if (!targetType)
        {
            m_ctx->getDiagCollector()->error("MirLegalizer",
                                             "Invalid target type for instruction's operand slot: {}",
                                             slot)
                    << inst->getSourceRef();
            return LegalizationResult::Failed;
        }

        switch (action)
        {
            case LegalizeAction::Legal:
                break;
            case LegalizeAction::WidenScalar:
                return LegalizeActions::LegalizeWidenScalar(ctx, slot, targetType);
            case LegalizeAction::NarrowScalar:
                return LegalizeActions::LegalizeNarrowScalar(ctx, slot, targetType);
            case LegalizeAction::Bitcast:
                return LegalizeActions::LegalizeBitcast(ctx, slot, targetType);
            case LegalizeAction::Libcall:
            {
                std::string_view sym = getLibcallSymbol(inst);
                if (!sym.empty())
                {
                    return LegalizeActions::LegalizeLibcall(ctx, sym);
                }

                m_ctx->getDiagCollector()->error("MirLegalizer", "Invalid liball for instruction")
                        << inst->getSourceRef();
                break;
            }
            case LegalizeAction::Custom:
                // This should not trigger and tryExpand should have already fired the rule, but just in case.
                return LegalizeActions::LegalizeCustom(ctx);
            case LegalizeAction::Unsupported:
                return LegalizationResult::Failed;
        }
    }

    return LegalizationResult::NotModified;
}

LegalizeAction MirLegalizer::getTargetLegalizeAction(const MirInstruction *inst, size_t operandSlot) const
{
    return LegalizeAction::Unsupported;
}

MirType *MirLegalizer::getTargetLegalType(const MirInstruction *inst, size_t operandSlot) const { return nullptr; }

std::string_view MirLegalizer::getLibcallSymbol(const MirInstruction *inst) const
{
    if (!inst)
    {
        return {};
    }

    return {};
}
