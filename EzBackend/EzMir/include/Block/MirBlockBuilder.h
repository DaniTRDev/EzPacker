#ifndef EZPACKER_MIRBLOCKBUILDER_H
#define EZPACKER_MIRBLOCKBUILDER_H

#include "EzCoreCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Instruction/MirInstructionBuilder.h"

class MirBlockBuilder : public MirBuilder<MirBlock>
{
  public:
    /**
     * Creates a block builder attached to the given owning list.
     * @param ctx
     */
    MirBlockBuilder(MirBuilderContext *ctx, std::pmr::list<MirBlock *> *owner);

    /**
     * Creates a block builder attached to the given func.
     * @param ctx
     */
    MirBlockBuilder(MirBuilderContext *ctx, MirFunction *owner);

    /**
     * Builds a block returns it. This function does not change owner.
     * @param sourceRef
     * @param name
     * @return
     */
    MirBlock *build(SourceReference *sourceRef = nullptr, const std::pmr::string &name = "");

    /**
     * Returns an instruction builder linked to the current block and context. This needs "build" to have been called,
     * will push a diagnostic error if not and will return an invalid builder.
     * @param opcode
     * @return
     */
    MirInstructionBuilder instrBuilder();

  private:
    MirBuilderContext *m_ctx;
    MirFunction *m_ownerFunc;
    std::pmr::list<MirBlock *> *m_owner;
    MirInstructionInsertionPoint m_insertPoint; // The insertion point of the created block.
};

#endif // EZPACKER_MIRBLOCKBUILDER_H
