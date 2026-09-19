#ifndef EZMIR_MIR_FUNCTION_BUILDER_H
#define EZMIR_MIR_FUNCTION_BUILDER_H

#include "EzMirCommon.h"
#include "Block/MirBlockBuilder.h"

/**
 * TODO: Remove this ugly list constructor and add a MirModule.
 */
/**
 * Fluent builder for constructing MirFunction instances.
 * Manages formal parameter accumulation, calling convention configuration,
 * entry block creation, and automatic registration into the owning module list.
 */
class MirFunctionBuilder : public MirBuilder<class MirFunction>
{
  public:
    /**
     * Constructs a function builder bound to a MirBuilderContext.
     */
    MirFunctionBuilder(class MirBuilderContext *ctx);

    /**
     * Constructs a function builder bound to a context and an owning function list.
     */
    MirFunctionBuilder(class MirBuilderContext *ctx, std::pmr::vector<class MirFunction *> *owner);

    /**
     * Creates and returns a MirBlockBuilder configured to append blocks to the LAST function built, if no function was
     * built, an invalid builder is returned.
     */
    MirBlockBuilder blockBuilder();

    /**
     * Adds a parameter to the END of the given function.
     */
    MirFunctionBuilder &addParam(MirFunction *func, class MirRegister *param);

    /**
     * Adds a parameter to the FRONT of the given function.
     */
    MirFunctionBuilder &addParamFront(MirFunction *func, class MirRegister *param);

    /**
     * Adds a physical register usage to the function.
     */
    MirFunctionBuilder &addPhysRegUse(MirFunction *func, const class MirRegisterRef &ref);

    /**
     * Finalizes and instantiates the MirFunction in the arena with the given calling conv, return type, name, and
     * source location. If calling convention is nullptr, the default one will be used (provided by the context)
     */
    MirFunction *build(class MirType *returnType,
                       std::initializer_list<MirRegister *> parameters,
                       const std::string_view &name = "",
                       class CallingConvDesc *cc = nullptr,
                       class SourceReference *sourceRef = nullptr);

  private:
    class MirBuilderContext *m_ctx;                 // Context providing the arena, diagnostics and registration.
    std::pmr::vector<class MirFunction *> *m_owner; // Optional external container the built function is appended to.
};

#endif // EZMIR_MIR_FUNCTION_BUILDER_H
