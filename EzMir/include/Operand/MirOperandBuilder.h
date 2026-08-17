#ifndef EZMIR_MIR_OPERAND_BUILDER_H
#define EZMIR_MIR_OPERAND_BUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "FlexNumber/FlexFloat.h"
#include "FlexNumber/FlexInt.h"
#include "Operand/MirOperand.h"

class MirOperandBuilder : public MirBuilder<MirOperand>
{
  public:
    /**
     * Creates the builder with the given context.
     */
    MirOperandBuilder(class MirBuilderContext *ctx);

    /**
     * Returns a float operand with the given floating point value. If the value's bit-width is smaller than given
     * type's, it will be extended and a warning will be thrown. If it is greater, given MirType will be
     * extended to the closest available and a warning will be thrown; if no mir type is available an error will be
     * thrown.
     *
     * If given type is not a floating type, an error will be thrown and nullptr will be returned.
     */
    class MirFloat *buildFloat(class MirType *type, const FlexFloat &value, class SourceReference *ref = nullptr);

    /**
     * Returns an integer operand with the given value. If the value's bit-width is smaller than given
     * type's, it will be z-extended (non-signed) and a warning will be thrown. If it is greater, given MirType will be
     * extended to the closest available and a warning will be thrown; if no mir type is available an error will be
     * thrown.
     *
     * If given type is not an integer type, an error will be thrown and nullptr will be returned.
     */
    class MirInteger *buildInt(class MirType *type, const FlexInt &value, class SourceReference *ref = nullptr);

    /**
     * Builds a memory operand out of the given parameters.
     *
     * If the given base is not a pointer, an error will be thrown an nullptr will be returned.
     */
    class MirMemory *buildMem(class MirType *type,
                              class MirRegister *base,
                              class MirInteger *displ,
                              class SourceReference *ref = nullptr);

    /**
     * Builds a memory operand out of the given parameters. A MirInteger is built out of the given displ int.
     *
     * If the given base is not a pointer, an error will be thrown an nullptr will be returned.
     */
    class MirMemory *
    buildMem(class MirType *type, class MirRegister *base, const FlexInt &displ, class SourceReference *ref = nullptr);

    /**
     * Creates a virtual register with the given type, name and source reference.
     */
    class MirRegister *buildVReg(MirType *type,
                                 std::pmr::string name = "",
                                 class SourceReference *ref = nullptr,
                                 class MirRegisterClass *_class = nullptr);

    /**
     * Creates a physical register with the given type, physical ID, name and source reference.
     */
    class MirRegister *buildPhysReg(class MirType *type,
                                    MirPhysicalRegId physId,
                                    std::pmr::string name = "",
                                    class MirRegisterClass *_class = nullptr,
                                    class SourceReference *ref = nullptr);
    /**
     * Creates a reference to the given block.
     */
    class MirReference *buildRef(class MirBlock *block, class SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given function.
     */
    class MirReference *buildRef(class MirFunction *func, class SourceReference *ref = nullptr);

    /**
     * Creates a reference to a global variable at given offset.
     */
    class MirReference *buildRef(class MirGlobalVar *var, size_t offset, class SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given class field.
     */
    class MirReference *
    buildRef(class MirRegister *classPtr, class MirClassField *field, class SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given class method.
     */
    class MirReference *
    buildRef(class MirRegister *classPtr, class MirClassMethod *method, class SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given stack frame object.
     */
    class MirReference *buildRef(class StackFrameObject *obj, class SourceReference *ref = nullptr);

    /**
     * Builds a runtime symbol that will later be resolved by the backend. A runtime symbol is a symbol that
     * has been defined within the runtime library.
     */
    class MirRuntimeSymbol *buildRtSymbol(std::pmr::string symbolName, class SourceReference *ref = nullptr);

  private:
    /**
     * Creates the operand with the given context and args. OperandType must be a sub type of MirOperand.
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
    MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_resource;
    std::pmr::polymorphic_allocator<> m_allocator;
};

#endif // EZMIR_MIR_OPERAND_BUILDER_H
