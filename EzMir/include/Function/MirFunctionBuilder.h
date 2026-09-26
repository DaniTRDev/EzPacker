#ifndef EZMIR_MIR_FUNCTION_BUILDER_H
#define EZMIR_MIR_FUNCTION_BUILDER_H

#include "EzMirCommon.h"
#include "Block/MirBlockBuilder.h"

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
     * Finalizes and instantiates the MirFunction in the arena with the given calling conv, return type, name,
     * linkage, and source location. If calling convention is nullptr, the default one will be used (provided by the context).
     */
    MirFunction *build(class MirType *returnType,
                       std::initializer_list<MirRegister *> parameters,
                       const std::string_view &name = "",
                       MirLinkage linkage = MirLinkage::External,
                       class CallingConvDesc *cc = nullptr,
                       class SourceReference *sourceRef = nullptr);

    /**
     * Backward-compatible overload accepting calling convention before source reference and linkage.
     */
    MirFunction *build(class MirType *returnType,
                       std::initializer_list<MirRegister *> parameters,
                       const std::string_view &name,
                       class CallingConvDesc *cc,
                       class SourceReference *sourceRef = nullptr,
                       MirLinkage linkage = MirLinkage::External);

    /**
     * Declares an external function prototype without a body (no basic blocks),
     * similar to an 'extern' function declaration in C.
     */
    MirFunction *declare(class MirType *returnType,
                         std::initializer_list<MirRegister *> parameters,
                         const std::string_view &name = "",
                         MirLinkage linkage = MirLinkage::External,
                         class CallingConvDesc *cc = nullptr,
                         class SourceReference *sourceRef = nullptr);

    /**
     * Declares an external function prototype from a list of parameter types without a body.
     */
    MirFunction *declare(class MirType *returnType,
                         std::initializer_list<class MirType *> parameterTypes,
                         const std::string_view &name = "",
                         MirLinkage linkage = MirLinkage::External,
                         class CallingConvDesc *cc = nullptr,
                         class SourceReference *sourceRef = nullptr);

    /**
     * Declares an external function prototype from a span/vector of parameter types without a body.
     */
    MirFunction *declare(class MirType *returnType,
                         std::span<class MirType *const> parameterTypes,
                         const std::string_view &name = "",
                         MirLinkage linkage = MirLinkage::External,
                         class CallingConvDesc *cc = nullptr,
                         class SourceReference *sourceRef = nullptr);

  private:
    class MirBuilderContext *m_ctx; // Context providing the arena, diagnostics and registration.
};

#endif // EZMIR_MIR_FUNCTION_BUILDER_H
