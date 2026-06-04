#ifndef EZPACKER_MIROPERANDBUILDER_H
#define EZPACKER_MIROPERANDBUILDER_H

#include "EzCoreCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"

class MirOperandBuilder : public MirBuilder<MirOperand>
{
  public:
    /**
     * Creates the builder with the given context. OperandType must be a sub type of MirOperand.
     * @param ctx
     */
    MirOperandBuilder(MirBuilderContext *ctx);

    /**
     * Flushes the operand.
     */
    ~MirOperandBuilder() override;

    /**
     * Creates the operand with the given context and args. OperandType must be a sub type of MirOperand.
     * @param ctx
     * @tparam OperandType
     * @tparam Args
     */
    template <typename OperandType, typename... Args>
        requires(std::is_base_of<MirOperand, OperandType>::value)
    OperandType *build(Args &&...args)
    {
        std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
        std::pmr::polymorphic_allocator<OperandType> alloc(arena);

        // Construct in-place, passing the arena down to the instruction's internal PMR vector
        OperandType *op = alloc.allocate(1);
        alloc.construct(op, std::forward<Args>(args)...);

        setBuildResult((MirOperand *)op);
        return op;
    }

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_MIROPERANDBUILDER_H
