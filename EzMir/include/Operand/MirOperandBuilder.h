#ifndef EZMIR_MIR_OPERAND_BUILDER_H
#define EZMIR_MIR_OPERAND_BUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "FlexNumber/FlexFloat.h"
#include "FlexNumber/FlexInt.h"
#include "Operand/MirOperand.h"

/**
 * Factory builder for constructing and allocating all variants of MirOperand objects
 * within the MirBuilderContext memory arena.
 */
class MirOperandBuilder : public MirBuilder<MirOperand>
{
  public:
    /**
     * Initializes the operand builder attached to the compilation context.
     */
    MirOperandBuilder(class MirBuilderContext *ctx);

    /**
     * Builds a floating-point constant operand (MirFloat).
     * If the value's bit-width is smaller than the target type, it is extended with a diagnostic warning.
     * If greater, the type is promoted to the closest matching floating-point type; if none matches, an error is
     * reported.
     */
    class MirFloat *buildFloat(class MirType *type, const FlexFloat &value, class SourceReference *ref = nullptr);

    /**
     * Builds an integer constant operand (MirInteger).
     * If the value's bit-width is smaller than the target type, it is zero-extended with a diagnostic warning.
     * If greater, the type is promoted to the closest matching integer type; if none matches, an error is reported.
     */
    class MirInteger *buildInt(class MirType *type, const FlexInt &value, class SourceReference *ref = nullptr);

    /**
     * Builds a memory address operand [base + displ] with an existing MirInteger displacement.
     * Validates that the base register operand has pointer type.
     */
    class MirMemory *buildMem(class MirType *type,
                              class MirRegister *base,
                              class MirInteger *displ,
                              class SourceReference *ref = nullptr);

    /**
     * Builds a memory address operand [base + displ] converting an immediate FlexInt displacement into a MirInteger.
     * Validates that the base register operand has pointer type.
     */
    class MirMemory *
    buildMem(class MirType *type, class MirRegister *base, const FlexInt &displ, class SourceReference *ref = nullptr);

    /**
     * Allocates a new virtual register operand (MirRegister) with a unique MIR ID,
     * registering it in the context register tracking list.
     */
    class MirRegister *buildVReg(MirType *type,
                                 std::string_view name = "",
                                 class SourceReference *ref = nullptr,
                                 class MirRegisterClass *_class = nullptr);

    /**
     * Allocates a physical hardware register operand (MirRegister) with a target physical register ID and class.
     */
    class MirRegister *buildPhysReg(class MirType *type,
                                    MirPhysicalRegId physId,
                                    std::string_view name = "",
                                    class MirRegisterClass *_class = nullptr,
                                    class SourceReference *ref = nullptr);

    /**
     * Builds a symbolic reference operand pointing to a MirBlock label.
     */
    class MirReference *buildRef(class MirBlock *block, class SourceReference *ref = nullptr);

    /**
     * Builds a symbolic reference operand pointing to a MirFunction entry point.
     */
    class MirReference *buildRef(class MirFunction *func, class SourceReference *ref = nullptr);

    /**
     * Builds a symbolic reference operand pointing to a MirGlobalVar at the given byte offset.
     */
    class MirReference *buildRef(class MirGlobalVar *var, size_t offset, class SourceReference *ref = nullptr);

    /**
     * Builds a symbolic reference operand pointing to a StackFrameObject slot.
     */
    class MirReference *buildRef(class StackFrameObject *obj, class SourceReference *ref = nullptr);

    /**
     * Builds a named external runtime symbol reference operand (e.g. "@__ez_rt_alloc").
     */
    class MirRuntimeSymbol *buildRtSymbol(std::pmr::string symbolName, class SourceReference *ref = nullptr);

  private:
    /**
     * Internal generic factory allocating concrete OperandType in the arena allocator.
     */
    template <typename OperandType, typename... Args>
        requires(std::is_base_of<MirOperand, OperandType>::value)
    OperandType *build(Args &&...args)
    {
        // Construct in-place, passing the arena down to the instruction's internal PMR vector
        OperandType *op = m_allocator.template new_object<OperandType>(std::forward<Args>(args)...);
        setBuildResult(static_cast<MirOperand *>(op));
        return op;
    }

  private:
    /**
     * MIR compilation context.
     */
    MirBuilderContext *m_ctx;

    /**
     * PMR memory resource for allocations.
     */
    std::pmr::memory_resource *m_resource;

    /**
     * Polymorphic allocator instance.
     */
    std::pmr::polymorphic_allocator<> m_allocator;
};

#endif // EZMIR_MIR_OPERAND_BUILDER_H
