#ifndef EZMIR_MIR_BLOCK_BUILDER_H
#define EZMIR_MIR_BLOCK_BUILDER_H

#include "EzCoreCommon.h"
#include "Instruction/MirInstructionBuilder.h"

class MirBlockBuilder : public MirBuilder<class MirBlock>
{
  public:
    /**
     * Creates a block builder attached to the given func.
     */
    MirBlockBuilder(class MirBuilderContext *ctx, class MirFunction *owner);

    /**
     * Builds a block and returns it. After this has been called and returned non-nullptr, instrBuilder can be used.
     */
    MirBlock *build(class SourceReference *sourceRef = nullptr, const std::pmr::string &name = "");

    /**
     * Returns an instruction builder linked to the current block and context. This needs "build" to have been called,
     * will push a diagnostic error if not and will return an invalid builder.
     */
    MirInstructionBuilder instrBuilder();

  private:
    class MirBuilderContext *m_ctx;
    class MirFunction *m_ownerFunc;
    MirInstructionInsertionPoint m_insertPoint; // The insertion point of the created block.
};

#endif // EZMIR_MIR_BLOCK_BUILDER_H
