#ifndef EZMIR_MIR_BLOCK_BUILDER_H
#define EZMIR_MIR_BLOCK_BUILDER_H

#include "EzCoreCommon.h"
#include "Instruction/MirInstructionBuilder.h"

/**
 * Builder creating MirBlock instances in the context memory arena,
 * wiring them into the parent function's CFG and providing sub-builders for instruction emission.
 */
class MirBlockBuilder : public MirBuilder<class MirBlock>
{
  public:
    /**
     * Constructs a block builder attached to the context and owning function.
     */
    MirBlockBuilder(class MirBuilderContext *ctx, class MirFunction *owner);

    /**
     * Allocates and initializes a new basic block with optional source reference and label name,
     * appending it to the builder context and owning function.
     */
    MirBlock *build(class SourceReference *sourceRef = nullptr, const std::pmr::string &name = "");

    /**
     * Creates an instruction builder configured to append instructions into this built basic block.
     */
    MirInstructionBuilder instrBuilder();

  private:
    /**
     * Context providing memory arena and global tracking.
     */
    class MirBuilderContext *m_ctx;

    /**
     * Parent function receiving the created basic block.
     */
    class MirFunction *m_ownerFunc;

    /**
     * Insertion point initialized to append to the end of this block.
     */
    MirInstructionInsertionPoint m_insertPoint;
};

#endif // EZMIR_MIR_BLOCK_BUILDER_H
