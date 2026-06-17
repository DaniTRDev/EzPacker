#ifndef EZPACKER_MIROPERANDBUILDER_H
#define EZPACKER_MIROPERANDBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"

class MirOperandBuilder : public MirBuilder<MirOperand>
{
  public:
    /**
     * Creates the builder with the given context.
     * @param ctx
     */
    MirOperandBuilder(MirBuilderContext *ctx);

    /**
     * Returns a float with the given 32-bit value.
     * @param value
     * @param ref
     * @return
     */
    MirFloat *buildFloat(float value, SourceReference *ref = nullptr);

    /**
     * Returns a float with the given 64-bit value.
     * @param value
     * @param ref
     * @return
     */
    MirFloat *buildFloat(double value, SourceReference *ref = nullptr);

    /**
     * Returns a float with the given value. This function WILL check that the given type is indeed a floating
     * point type and will: return nullptr if it's not and send an error to the diagnostic collector.
     * @param type
     * @param value
     * @param ref
     * @return
     */
    MirFloat *buildFloat(MirType *type, std::pmr::string value, SourceReference *ref = nullptr);

    /**
     * Creates an integer with the given value.
     * @param type
     * @param value
     * @return
     */
    MirInteger *buildInt(MirType *type, int64_t value, SourceReference *ref = nullptr);

    /**
     * Creates a virtual register with the given type, name and source reference.
     * @param type
     * @param name
     * @param ref
     * @return
     */
    MirRegister *buildVReg(MirType *type, std::pmr::string name = "", SourceReference *ref = nullptr);

    /**
     * Creates a physical register with the given type, name and source reference.
     * @param type
     * @param name
     * @param ref
     * @return
     */
    MirRegister *buildPhysReg(MirType *type, std::pmr::string name = "", SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given block.
     * @param block
     * @param ref
     * @return
     */
    MirReference *buildRef(MirBlock *block, SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given function.
     * @param func
     * @param ref
     * @return
     */
    MirReference *buildRef(MirFunction *func, SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given data entry
     * @param entry
     * @param ref
     */
    MirReference *buildRef(MirGlobalDataEntry *entry, SourceReference *ref = nullptr);

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
        std::pmr::polymorphic_allocator alloc(arena);

        // Construct in-place, passing the arena down to the instruction's internal PMR vector
        OperandType *op = alloc.template new_object<OperandType>(std::forward<Args>(args)...);
        setBuildResult(static_cast<MirOperand *>(op));

        if constexpr (std::is_same<OperandType, MirRegister>::value)
        {
            // If the operand is a register, ensure we add it to the register list of the context.
            m_ctx->appendRegister(op);
        }

        return op;
    }

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_MIROPERANDBUILDER_H
