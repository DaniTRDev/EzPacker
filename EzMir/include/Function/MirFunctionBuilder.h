#ifndef EZMIR_MIR_FUNCTION_BUILDER_H
#define EZMIR_MIR_FUNCTION_BUILDER_H

#include "EzMirCommon.h"
#include "Block/MirBlockBuilder.h"

/**
 * TODO: Remove this ugly list constructor and add a MirModule.
 */
class MirFunctionBuilder : public MirBuilder<class MirFunction>
{
  public:
    /**
     * Creates the function builder with the given context.
     */
    MirFunctionBuilder(class MirBuilderContext *ctx);

    /**
     * Creates the function builder linked to an owner vector container.
     */
    MirFunctionBuilder(class MirBuilderContext *ctx, std::pmr::vector<class MirFunction *> *owner);

    /**
     * Returns a block builder attached to the current function. If this function HAS NOT been built, an invalid
     * block builder is returned and a diagnostic error is pushed.
     */
    MirBlockBuilder blockBuilder();

    /**
     * Builds a function over arena-managed MIR data structures.
     */
    MirFunction *
    build(class MirType *returnType, const std::pmr::string &name = "", class SourceReference *sourceRef = nullptr);

    /**
     * Adds a parameter into the FUTURE function that's going to be built. This does not affect the stack frame.
     */
    MirFunctionBuilder &
    buildParam(class MirType *type, const std::pmr::string &name = "", class SourceReference *sourceRef = nullptr);

    /**
     * Appends the given already-built parameter into the function.
     */
    MirFunctionBuilder &buildParam(class MirRegister *param);

    /**
     * Sets the calling convention of the FUTURE function that's going to be built.
     */
    MirFunctionBuilder &setCallingConvention(class CallingConvDesc *cc);

  private:
    class CallingConvDesc *m_callingConv;
    class MirBuilderContext *m_ctx;
    std::pmr::list<class MirRegister *> m_parameters;
    std::pmr::vector<class MirFunction *> *m_owner;
};

#endif // EZMIR_MIR_FUNCTION_BUILDER_H
