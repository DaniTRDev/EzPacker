#include "Block/MirBlockBuilder.h"

MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, std::pmr::list<MirBlock *> *owner) : m_ctx(ctx), m_owner(owner)
{
    if (!owner->empty())
    {
        MirBlock *targetBlock = owner->back();
        m_insertPoint = { .m_type = InsertionType::InsertAfter,
                          .m_block = targetBlock,
                          .m_iterator = targetBlock->getInstructions().begin() };
    }
}

MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner) :
    MirBlockBuilder(ctx, owner->getBlocksPtr())
{
}

MirBlock *MirBlockBuilder::build(SourceReference *sourceRef, const std::pmr::string &name)
{
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirBlock *block =
            alloc.new_object<MirBlock>(m_ctx->createId(), sourceRef, std::pmr::list<MirInstruction *>(alloc), name);

    m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirBlockBuilder")
            << sourceRef << std::pmr::string(std::format("Built block with id: {}", block->getId()));

    if (m_ctx->appendBlock(block))
    {
        m_insertPoint = { .m_type = InsertionType::InsertAfter,
                          .m_block = block,
                          .m_iterator = block->getInstructions().begin() };

        m_owner->push_back(block); // InsertAfter the block to the owner function.

        setBuildResult(block);
        return block;
    }

    return nullptr;
}

MirInstructionBuilder MirBlockBuilder::instrBuilder()
{
    if (!getBuiltObj())
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirBlockBuilder")
                << "Can't create an instruction builder for a block if block was not built";
        return MirInstructionBuilder(nullptr, {});
    }

    return MirInstructionBuilder(m_ctx, m_insertPoint);
}
