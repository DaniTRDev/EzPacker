#include "Builder/MirBuilderContext.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
#include "Function/MirFunction.h"
#include "SourceManager/SourceManager.h"

MirBlockBuilder::MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner) : m_ctx(ctx), m_ownerFunc(owner) {}

MirBlock *MirBlockBuilder::build(SourceReference *sourceRef, const std::pmr::string &name)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirBlock *block = alloc.new_object<MirBlock>(m_ctx->createId(),
                                                 sourceRef,
                                                 std::pmr::list<MirInstruction *>(alloc),
                                                 m_ownerFunc,
                                                 name);

    m_ctx->getDiagCollector()->builder(Diag_Debug, "MirBlockBuilder")
            << sourceRef << std::pmr::string(std::format("Built block with id: {}", block->getId()));

    if (m_ctx->appendBlock(block))
    {
        m_insertPoint = { .m_type = InsertionType::InsertAfter,
                          .m_block = block,
                          .m_iterator = block->getInstructions().begin() };

        m_ownerFunc->getBlocks().push_back(block);
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
