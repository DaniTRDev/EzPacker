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
#include "Legalizer/InsertionTracker.h"
#include "Legalizer/LegalizerInfo.h"
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
    if (!block || !m_ctx)
        return false;

    // 1. Initialize worklist with all instructions in the block in reverse order
    // so popping from the back processes instructions in forward sequence.
    std::pmr::vector<MirInstruction *> worklist(m_ctx->getGlobalAllocator());
    worklist.reserve(block->getInstructions().size());
    for (auto it = block->getInstructions().rbegin(); it != block->getInstructions().rend(); ++it)
    {
        worklist.push_back(*it);
    }

    // 2. Cycle detection budget (proportional to block size)
    const size_t maxSteps = worklist.size() * 32 + 256;
    size_t stepsTaken = 0;

    while (!worklist.empty())
    {
        if (++stepsTaken > maxSteps)
        {
            m_ctx->getDiagCollector()->error(
                "MirLegalizer",
                "Infinite legalization cycle detected in block '{}'",
                block->getName());
            return false;
        }

        MirInstruction *inst = worklist.back();
        worklist.pop_back();

        // Skip instructions erased by prior lowering actions
        if (!inst || inst->isErased())
            continue;

        // 3. Formulate Legality Query
        LegalityQuery query = buildQuery(inst);

        // 4. Query target legality
        LegalityResponse response;
        if (m_targetDesc && m_targetDesc->getLegalizerInfo())
        {
            response = m_targetDesc->getLegalizerInfo()->query(query);
        }

        // Fast path: instruction is already legal
        if (response.isLegal())
            continue;

        if (response.isUnsupported())
        {
            m_ctx->getDiagCollector()->error(
                "MirLegalizer",
                "Unsupported instruction '{}' with operand type(s)",
                inst->getOpCodeName()) << inst->getSourceRef();
            return false;
        }

        // 5. Track insertion of new instructions during transformation
        InsertionTracker tracker(block, inst);
        LegalizeCtx ctx(m_ctx, m_targetDesc, tracker.getIterator());

        LegalizationResult result = executeAction(response, ctx, inst);
        if (result == LegalizationResult::Failed)
        {
            return false;
        }

        // 6. Push newly synthesized instructions onto worklist in reverse order for recursive verification
        auto produced = tracker.getProducedInstructions();
        for (auto it = produced.rbegin(); it != produced.rend(); ++it)
        {
            worklist.push_back(*it);
        }
    }

    return true;
}

LegalizationResult MirLegalizer::legalizeInstruction(IntrusiveLinkedList<MirInstruction>::iterator it, MirBlock *block)
{
    if (!m_ctx)
        return LegalizationResult::Failed;

    MirInstruction *inst = *it;
    if (!inst || inst->isErased())
        return LegalizationResult::NotModified;

    LegalityQuery query = buildQuery(inst);

    LegalityResponse response;
    if (m_targetDesc && m_targetDesc->getLegalizerInfo())
    {
        response = m_targetDesc->getLegalizerInfo()->query(query);
    }

    if (response.isLegal())
    {
        return LegalizationResult::NotModified;
    }

    if (response.isUnsupported())
    {
        m_ctx->getDiagCollector()->error("MirLegalizer",
                                         "Unsupported instruction '{}'",
                                         inst->getOpCodeName())
                << inst->getSourceRef();
        return LegalizationResult::Failed;
    }

    LegalizeCtx ctx(m_ctx, m_targetDesc, it);
    return executeAction(response, ctx, inst);
}

LegalityQuery MirLegalizer::buildQuery(MirInstruction *inst)
{
    LegalityQuery q;
    if (!inst)
        return q;

    q.m_opcode = inst->getOpCode();
    q.m_flags = static_cast<uint32_t>(inst->getFlags());
    q.m_operandCount = inst->getOperandCount();

    size_t limit = std::min(inst->getOperandCount(), q.m_types.size());
    for (size_t i = 0; i < limit; ++i)
    {
        MirOperand *op = inst->getOperand(i);
        if (op)
        {
            q.m_types[i] = op->getMirType();
            if (op->getMirType())
            {
                q.m_compactIds[i] = op->getMirType()->getCompactId();
            }

            if (op->isOfType<MirRegister>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::Register;
            }
            else if (op->isOfType<MirInteger>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::Integer;
                if (!q.m_hasImm)
                {
                    q.m_hasImm = true;
                    q.m_immValue = op->get<MirInteger>()->getValue().getI64();
                }
            }
            else if (op->isOfType<MirFloat>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::FloatingPoint;
            }
            else if (op->isOfType<MirMemory>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::Memory;
            }
            else if (op->isOfType<MirReference>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::Reference;
            }
            else if (op->isOfType<MirRuntimeSymbol>())
            {
                q.m_operandKinds[i] = ExpectedOperandType::RuntimeSymbol;
            }
        }
    }
    return q;
}

LegalizationResult MirLegalizer::executeAction(const LegalityResponse &response, LegalizeCtx &ctx, MirInstruction *inst)
{
    MirType *targetType = nullptr;
    if (response.m_targetCompactId != 0)
    {
        targetType = m_ctx->getTypeTable()->getTypeByCompactId(response.m_targetCompactId);
    }

    switch (response.m_action)
    {
        case LegalizeActionKind::Legal:
            return LegalizationResult::NotModified;

        case LegalizeActionKind::WidenScalar:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for WidenScalar on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeWidenScalar(ctx, response.m_slot, targetType);
        }

        case LegalizeActionKind::NarrowScalar:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for NarrowScalar on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeNarrowScalar(ctx, response.m_slot, targetType);
        }

        case LegalizeActionKind::Bitcast:
        {
            if (!targetType)
            {
                m_ctx->getDiagCollector()->error("MirLegalizer",
                                                 "Target type not found for Bitcast on opcode '{}'",
                                                 inst->getOpCodeName())
                        << inst->getSourceRef();
                return LegalizationResult::Failed;
            }
            return LegalizeActions::LegalizeBitcast(ctx, response.m_slot, targetType);
        }

        case LegalizeActionKind::Libcall:
        {
            std::string_view sym;
            if (m_targetDesc && m_targetDesc->getLegalizerInfo())
            {
                sym = m_targetDesc->getLegalizerInfo()->getLibcallSymbol(response.m_handlerOrStringId);
            }
            if (sym.empty() && m_targetDesc)
            {
                sym = m_targetDesc->getLibcallStr(static_cast<uint8_t>(response.m_handlerOrStringId));
            }
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

        case LegalizeActionKind::Lower:
        case LegalizeActionKind::Custom:
        {
            if (m_targetDesc && m_targetDesc->getLegalizerInfo())
            {
                return m_targetDesc->getLegalizerInfo()->executeCustom(ctx, response.m_handlerOrStringId);
            }
            return LegalizationResult::Failed;
        }

        case LegalizeActionKind::Unsupported:
        default:
            m_ctx->getDiagCollector()->error("MirLegalizer",
                                             "Unsupported action for opcode '{}'",
                                             inst->getOpCodeName())
                    << inst->getSourceRef();
            return LegalizationResult::Failed;
    }
}