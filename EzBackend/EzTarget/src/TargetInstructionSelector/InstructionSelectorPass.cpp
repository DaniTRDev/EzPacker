#include "TargetInstructionSelector/InstructionSelectorPass.h"

InstructionSelectorPass::InstructionSelectorPass(InstructionSelectionContext *ctx) : m_ctx(ctx) {}

bool InstructionSelectorPass::run(TypedPoolLinkedList<MirInstruction> *instrList,
                                  TypedPoolLinkedList<MirInstruction>::Iterator it,
                                  MirPassManager *passManager)
{
    MirInstruction *instr = *it;

    // Skip if already selected (e.g., manually lowered during ABI lowering)
    if (instr->getTargetId() != TARGET_INSTR_SELECT_NONE)
    {
        return false;
    }

    const InstructionSelectionTable *table = m_ctx->getSelectionTable();

    MirTargetInstructionId targetId = table->select(instr);
    if (targetId != TARGET_INSTR_SELECT_NONE)
    {
        instr->setTargetId(targetId);
        return true; // Modified (State changed)
    }

    // Fatal Error: The backend doesn't know how to map this generic instruction.
    m_ctx->getEmitter()->getContext()->emitError(
            ErrorSeverity::Fatal,
            std::format("Instruction Selection failed for generic opcode ID: {}", static_cast<int>(instr->getOpCode())),
            "InstructionSelectorPass::run");

    return false;
}

MirPassIterationPlace InstructionSelectorPass::getIterationPlace() const { return MirPassIterationPlace::Instruction; }

const char *InstructionSelectorPass::getName() const { return "InstructionSelectionPass"; }
