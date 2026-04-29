#include "MirInstructionLegalizer/MirInstructionLegalizerPass.h"

MirInstructionLegalizer::MirInstructionLegalizer(MirLegalizerContext *ctx) : m_ctx(ctx) {}

bool MirInstructionLegalizer::run(MirFunction *func, struct MirPassManager *passManager)
{
    bool changed = false;
    for (MirBlock *block : *func->getBlocks())
    {
        changed |= runOnBlock(block);
    }
    return changed;
}

bool MirInstructionLegalizer::runOnBlock(MirBlock *block)
{
    bool changed = false;

    TypedPoolSlice<MirInstruction> *instrList = block->getInstructions();
    for (TypedPoolSlice<MirInstruction>::Iterator it = instrList->begin(); it != instrList->end(); ++it)
    {
        changed |= runOnInstruction(it, block);
    }

    return changed;
}

bool MirInstructionLegalizer::runOnInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock)
{
    MirInstruction *instr = *instrIt;
    MirInstructionOpCode opcode = instr->getOpCode();

    if (opcode == MirInstructionOpCode::LOWERED)
    {
        // Don't touch this instruction.
        return false;
    }

    return false;
}
