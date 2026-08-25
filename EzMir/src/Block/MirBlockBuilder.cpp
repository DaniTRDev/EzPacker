#include "Block/MirBlockBuilder.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "SourceManager/SourceManager.h"

MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner) : m_ctx(ctx), m_ownerFunc(owner) {}

MirBlock *MirBlockBuilder::build(SourceReference *sourceRef, const std::pmr::string &name)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place without node wrapper allocations
    MirBlock *block = alloc.new_object<MirBlock>(m_ctx->createId(), sourceRef, m_ownerFunc, name);

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

MirInstructionBuilder MirBlockBuilder::instrBuilder()
{
    if (!getBuiltObj())
    {
        m_ctx->getDiagCollector()->error("MirBlockBuilder",
                                         "Can't create an instruction builder for a block if block was not built");
        return MirInstructionBuilder(nullptr, {});
    }

    return MirInstructionBuilder(m_ctx, m_insertPoint);
}