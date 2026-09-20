#include "Block/MirBlockBuilder.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "SourceManager/SourceManager.h"

/**
 * Initializes the block builder with parent context and owning function.
 */
MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner) : m_ctx(ctx), m_ownerFunc(owner) {}

/**
 * Allocates and builds a new basic block in the arena, records it with context and function,
 * and sets up default append insertion point.
 */
MirBlock *MirBlockBuilder::build(SourceReference *sourceRef, const std::string_view &name)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place without node wrapper allocations
    std::pmr::string pmrName(name, arena);
    MirBlock *block = alloc.new_object<MirBlock>(m_ctx->createId(), sourceRef, m_ownerFunc, arena, std::move(pmrName));

    m_ctx->getDiagCollector()->trace("MirBlockBuilder", "Built block with id: {}", block->getId()) << sourceRef;

    if (m_ctx->appendBlock(block))
    {
        m_insertPoint = { .m_type = InsertionType::Append,
                          .m_block = block,
                          .m_iterator = block->getInstructions().end() };

        if (m_ownerFunc)
        {
            m_ownerFunc->appendBlock(block);
        }

        setBuildResult(block);

        return block;
    }

    return nullptr;
}

/**
 * Creates an instruction builder configured with the insertion point of this basic block.
 */
MirInstructionBuilder MirBlockBuilder::instrBuilder()
{
    if (!getBuiltObj())
    {
        m_ctx->getDiagCollector()->error("MirBlockBuilder",
                                         "Can't create an instruction builder for a block if block was not built");
        // Keep the context so the returned builder emits unlinked instructions instead of
        // dereferencing a null context.
        return MirInstructionBuilder(m_ctx, static_cast<MirBlock *>(nullptr), InsertionType::Append);
    }

    return MirInstructionBuilder(m_ctx, m_insertPoint);
}

/**
 * Unlinks the block from its owning function's block list without freeing it in the arena.
 */
void MirBlockBuilder::erase(MirBlock *block)
{
    if (!block)
    {
        return;
    }

    if (auto *owner = block->getOwner())
    {
        owner->m_blocks.remove(block);
    }
}