#ifndef EZPACKER_MIROPERANDBUILDER_H
#define EZPACKER_MIROPERANDBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Class/MirClass.h"
#include "GlobalVar/MirGlobalVar.h"

class MirOperandBuilder : public MirBuilder<MirOperand>
{
  public:
    /**
     * Creates the builder with the given context.
     * @param ctx
     */
    MirOperandBuilder(MirBuilderContext *ctx);

    /**
     * This function will build a constant array with the given element type. Will check if the elements have the same
     * type as elemType and will return nullptr if there's a mismatch.
     * @param elemType
     * @param elems
     * @param ref
     */
    MirConstantArray *
    buildConstantArray(MirType *elemType, const std::vector<MirOperand *> &elems, SourceReference *ref = nullptr);

    /**
     * Returns a float operand with the given floating point value. If the value's bit-width is smaller than given
     * type's, it will be extended and a warning will be thrown. If it is greater, given MirType will be
     * extended to the closest available and a warning will be thrown; if no mir type is available an error will be
     * thrown.
     *
     * If given type is not a floating type, an error will be thrown and nullptr will be returned.
     * @param type
     * @param value
     * @param ref
     * @return
     */
    MirFloat *buildFloat(MirType *type, const FlexFloat &value, SourceReference *ref = nullptr);

    /**
     * Returns an integer operand with the given value. If the value's bit-width is smaller than given
     * type's, it will be z-extended (non-signed) and a warning will be thrown. If it is greater, given MirType will be
     * extended to the closest available and a warning will be thrown; if no mir type is available an error will be
     * thrown.
     *
     * If given type is not an integer type, an error will be thrown and nullptr will be returned.
     * @param type
     * @param value
     * @return
     */
    MirInteger *buildInt(MirType *type, const FlexInt &value, SourceReference *ref = nullptr);

    /**
     * Builds a memory operand out of the given parameters.
     * @param type
     * @param base
     * @param displ
     * @param ref
     * @return
     */
    MirMemory *buildMem(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref = nullptr);

    /**
     * Builds a memory operand out of the given parameters. A MirInteger is built out of the given displ int.
     * @param type
     * @param base
     * @param displ
     * @param ref
     * @return
     */
    MirMemory *buildMem(MirType *type, MirRegister *base, const FlexInt &displ, SourceReference *ref = nullptr);

    /**
     * Creates a virtual register with the given type, name and source reference.
     * @param type
     * @param name
     * @param ref
     * @return
     */
    MirRegister *buildVReg(MirType *type,
                           std::pmr::string name = "",
                           SourceReference *ref = nullptr,
                           MirRegisterClass *_class = nullptr);

    /**
     * Creates a physical register with the given type, physical ID, name and source reference.
     * @param type
     * @param physId
     * @param name
     * @param ref
     * @return
     */
    MirRegister *buildPhysReg(MirType *type,
                              PhysicalRegId physId,
                              std::pmr::string name = "",
                              MirRegisterClass *_class = nullptr,
                              SourceReference *ref = nullptr);

    /**
     * Creates an indexed reference to a specific element within a global array variable.
     * @param var The global variable target (must hold an Array type kind).
     * @param elementIndex The specific element index array slot accessed (e.g., array[5]).
     */
    MirReference *buildArrayElemRef(MirGlobalVar *var, size_t elementIndex, SourceReference *ref = nullptr);

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
     * Creates a reference to a global variable at given offset.
     * @param var
     * @param ref
     */
    MirReference *buildRef(MirGlobalVar *var, size_t offset, SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given class field.
     * @param classPtr
     * @param field
     */
    MirReference *buildRef(MirRegister *classPtr, MirClassField *field, SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given class method.
     * @param classPtr
     * @param field
     */
    MirReference *buildRef(MirRegister *classPtr, MirClassMethod *method, SourceReference *ref = nullptr);

    /**
     * Creates a reference to the given stack frame object.
     * @param obj
     * @param ref
     * @return
     */
    MirReference *buildRef(StackFrameObject *obj, SourceReference *ref = nullptr);

    /**
     * Builds a runtime symbol that will later be resolved by the backend. A runtime symbol is a symbol that
     * has been defined within the runtime library.
     * @param symbolName
     * @param ref
     * @return
     */
    MirRuntimeSymbol *buildRtSymbol(std::pmr::string symbolName, SourceReference *ref = nullptr);

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
