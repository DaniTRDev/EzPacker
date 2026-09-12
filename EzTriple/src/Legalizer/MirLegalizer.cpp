#include "Legalizer/MirLegalizer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Legalizer/Actions/LegalizeBitcastAction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeLibcallAction.h"
#include "Legalizer/Actions/LegalizeNarrowScalarAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/Actions/LegalizeWidenScalarAction.h"
#include "Legalizer/MirLegalizeActionTable.h"
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

    if (inst->getFlags() & MirInstructionFlags::IsCall)
    {
        return LegalizeActions::LegalizeCall(ctx);
    }
    if (inst->getFlags() & MirInstructionFlags::IsReturn)
    {
        return LegalizeActions::LegalizeReturn(ctx);
    }

    const auto *actionTable = m_targetDesc->getLegalizeActionTable();
    if (!actionTable)
    {
        m_ctx->getDiagCollector()->error("MirLegalizer", "Target does not provide a MirLegalizeActionTable")
                << inst->getSourceRef();
        return LegalizationResult::Failed;
    }

    // Extract compact type IDs for up to 3 operands (0 represents unset / default)
    uint8_t t0 = 0;
    uint8_t t1 = 0;
    uint8_t t2 = 0;

    size_t opCount = inst->getOperandCount();
    if (opCount > 0 && inst->getOperand(0) && inst->getOperand(0)->getMirType())
        t0 = inst->getOperand(0)->getMirType()->getCompactId();

    if (opCount > 1 && inst->getOperand(1) && inst->getOperand(1)->getMirType())
        t1 = inst->getOperand(1)->getMirType()->getCompactId();

    if (opCount > 2 && inst->getOperand(2) && inst->getOperand(2)->getMirType())
        t2 = inst->getOperand(2)->getMirType()->getCompactId();

    // Query the action table in O(1) time
    LegalizeQueryResult decision = actionTable->query(inst->getOpCode(), t0, t1, t2);

    /*
     * Fast path: instruction is natively supported. This intrinsic allows the compiler to optimize the cmp + jump to
     * ensure CPU's branch prediction does not waste its effort.
     */
    if (__builtin_expect(decision.m_action == LegalizeAction::Legal, 1))
    {
        return LegalizationResult::NotModified;
    }

    MirType *targetType = m_ctx->getTypeTable()->getTypeByCompactId(decision.m_compactId);
    switch (decision.m_action)
    {
        case LegalizeAction::Legal:
            return LegalizationResult::NotModified;

        case LegalizeAction::WidenScalar:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for WidenScalar on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeWidenScalar(ctx, decision.m_slot, targetType);
        }

        case LegalizeAction::NarrowScalar:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for NarrowScalar on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeNarrowScalar(ctx, decision.m_slot, targetType);
        }

        case LegalizeAction::Bitcast:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for Bitcast on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeBitcast(ctx, decision.m_slot, targetType);
        }

        case LegalizeAction::Libcall:
        {
            std::string_view sym = m_targetDesc->getLibcallStr(decision.m_libcallOffset);
            if (!sym.empty())
            {
                return LegalizeActions::LegalizeLibcall(ctx, sym);
            }

            m_ctx->getDiagCollector()->error("MirLegalizer",
                                             "Invalid libcall symbol for opcode '{}'",
                                             inst->getOpCodeName())
                    << inst->getSourceRef();
            return LegalizationResult::Failed;
        }

        case LegalizeAction::Custom:
            return actionTable->executeCustomAction(&ctx, decision.m_customActionId);

        case LegalizeAction::Unsupported:
        default:
            m_ctx->getDiagCollector()->error("MirLegalizer",
                                             "Unsupported type combination for opcode '{}' (t0: {}, t1: {}, t2: {})",
                                             inst->getOpCodeName(),
                                             t0,
                                             t1,
                                             t2)
                    << inst->getSourceRef();
            return LegalizationResult::Failed;
    }
}