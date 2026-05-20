#include "TargetLegalizer/TypeLegalizerPass.h"

TypeLegalizerPass::TypeLegalizerPass(LegalizerContext *ctx) : m_ctx(ctx) {}

bool TypeLegalizerPass::run(TypedPoolLinkedList<struct MirInstruction> *instrList,
                            TypedPoolLinkedList<struct MirInstruction>::Iterator it,
                            struct MirPassManager *passManager)
{
    MirEmitter *emitter = m_ctx->getEmitter();
    MirInstruction *instr = *it;
    MirInstructionOpCode opcode = instr->getOpCode();
    MirInstructionMetadata meta = instr->getMetadata();
    TargetDesc *targetDesc = m_ctx->getTargetDesc();
    TypedPoolLinkedList<MirOperand> *operands = instr->getOperands();

    // Check if any of the instruction's operands require legalization.
    size_t operandId = 0;
    for (auto operandIt = operands->begin(); operandIt != operands->end(); ++operandIt, operandId++)
    {
        MirOperand *op = *operandIt;
        uint8_t actionType = m_ctx->getActionList()->getOperandAction(opcode, op->getSizeInBytes() * 8);

        if (actionType != Action_None)
        {
            // If this instruction has a custom handler, call it to legalize the instruction.
            if (actionType & Action_TypeCustom)
            {
                bool res = false;
                for (auto handler : m_ctx->getHandlerList()->getInstructionHandlers(opcode))
                {
                    res |= handler(instrList, it).m_modified;
                }

                return res;
            }
            else
            {
                if (actionType & Action_PromoteOperand)
                {
                    return StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, operandIt, operandId);
                }

                if (actionType & Action_ExpandOperand)
                {
                    return StandardLegalizers::expandTypeLegalizer(m_ctx, instrList, it);
                }
            }
        }
    }

    return false;
}

const char *TypeLegalizerPass::getName() const { return "TypeLegalizerPass"; }

MirPassIterationPlace TypeLegalizerPass::getIterationPlace() const { return MirPassIterationPlace::Instruction; }
