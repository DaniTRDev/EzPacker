#include "Instruction/MirInstructionBuilder.h"

MirInstructionBuilder::MirInstructionBuilder(MirEmitterContext *ctx, MirInstructionOpCode opcode) : m_ctx(ctx)
{
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator<MirInstruction> alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    m_instr = alloc.allocate(1);
    alloc.construct(m_instr, opcode, std::pmr::vector<MirOperand *>(arena));
}

void MirInstructionBuilder::flush()
{
    if (m_ctx && m_instr)
    {
        auto point = m_ctx->getInsertPoint();
        if (point.mode == InsertMode::Append)
        {
            point.block->getInstructions().push_back(std::move(*m_instr));
        }
        else
        {
            point.block->getInstructions().insert(point.iterator, std::move(*m_instr));
        }

        m_ctx = nullptr;
        m_instr = nullptr;
    }
}

MirInstructionBuilder &MirInstructionBuilder::operator<<(const MirOperand &operand)
{
    m_instr->addOperand(operand);
    return *this;
}
