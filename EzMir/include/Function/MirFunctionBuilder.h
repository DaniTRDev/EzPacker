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
     * Creates and returns a MirBlockBuilder configured to append blocks to this function.
     */
    MirBlockBuilder blockBuilder();

    /**
     * Finalizes and instantiates the MirFunction in the arena with return type, name, and source location.
     */
    MirFunction *
    build(class MirType *returnType, const std::pmr::string &name = "", class SourceReference *sourceRef = nullptr);

    /**
     * Adds an incoming virtual register parameter to the function signature under construction.
     */
    MirFunctionBuilder &
    buildParam(class MirType *type, const std::pmr::string &name = "", class SourceReference *sourceRef = nullptr);

    /**
     * Appends an existing MirRegister parameter to the function under construction.
     */
    MirFunctionBuilder &buildParam(class MirRegister *param);

    /**
     * Sets the target calling convention descriptor for the function under construction.
     */
    MirFunctionBuilder &setCallingConvention(class CallingConvDesc *cc);

  private:
    class CallingConvDesc *m_callingConv;
    class MirBuilderContext *m_ctx;
    std::pmr::list<class MirRegister *> m_parameters;
    std::pmr::vector<class MirFunction *> *m_owner;
};

#endif // EZMIR_MIR_FUNCTION_BUILDER_H
