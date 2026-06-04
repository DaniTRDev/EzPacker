#include "Block/MirBlockBuilder.h"

MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner) : m_ctx(ctx), m_owner(owner) {}

MirBlockBuilder::~MirBlockBuilder() { MirBuilder::flush(); }

MirBlock *MirBlockBuilder::build(SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator<MirBlock> alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirBlock *block = alloc.allocate(1);
    alloc.construct(block, m_ctx->createId(), sourceRef, std::pmr::list<MirInstruction *>(alloc));

    m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirBlockBuilder")
            << sourceRef << std::pmr::string(std::format("Built block with id: {}", block->getId()));

    if (m_ctx->appendBlock(block))
    {
        m_insertPoint = { .m_type = InsertionType::Append,
                          .m_block = block,
                          .m_iterator = block->getInstructions().begin() };
        m_owner->getBlocks().push_back(block); // Append the block to the owner function.

        setBuildResult(block);
        return block;
    }

    return nullptr;
}

MirInstructionBuilder MirBlockBuilder::instrBuilder() { return MirInstructionBuilder(m_ctx, &m_insertPoint); }
