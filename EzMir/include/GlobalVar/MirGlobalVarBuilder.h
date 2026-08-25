#ifndef EZPACKER_MIRGLOBALVARBUILDER_H
#define EZPACKER_MIRGLOBALVARBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "GlobalVar/MirGlobalVar.h"

/**
 * Builder class for allocating and configuring MirGlobalVar instances within a MirBuilderContext.
 */
class MirGlobalVarBuilder : public MirBuilder<MirGlobalVar>
{
  public:
    /**
     * Constructs a builder associated with the target MIR compilation context.
     */
    MirGlobalVarBuilder(class MirBuilderContext *ctx);

    /**
     * Allocates and initializes a new MirGlobalVar in the context arena allocator.
     * Generates a unique MIR ID and logs trace diagnostics.
     */
    MirGlobalVar *build(MirGlobalVarLinkage linkage,
                        class MirType *type,
                        const std::pmr::string &name,
                        class SourceReference *sourceRef = nullptr);

    /**
     * Sets whether the constructed global variable is marked as constant (read-only).
     */
    MirGlobalVarBuilder &setConstant(bool constant);

    /**
     * Assigns the constant initializer operand (must be a constant type: MirInteger, MirFloat, or MirConstantArray).
     * Emits a diagnostic error if an invalid operand is supplied.
     */
    MirGlobalVarBuilder &setInitializer(class MirOperand *initializer);

  private:
    /**
     * Constant flag state for the global variable being built (defaults to true).
     */
    bool m_constant;

    /**
     * Context providing arena allocators, diagnostic collectors, and ID generators.
     */
    MirBuilderContext *m_ctx;

    /**
     * Initializer operand to attach to the global variable.
     */
    class MirOperand *m_initializer;
};

#endif // EZPACKER_MIRGLOBALVARBUILDER_H