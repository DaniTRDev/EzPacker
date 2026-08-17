#ifndef EZPACKER_MIRGLOBALVARBUILDER_H
#define EZPACKER_MIRGLOBALVARBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "GlobalVar/MirGlobalVar.h"

class MirGlobalVarBuilder : public MirBuilder<MirGlobalVar>
{
  public:
    /**
     * Creates the builder with the given context.
     */
    MirGlobalVarBuilder(class MirBuilderContext *ctx);

    /**
     * Creates the global variable with the given parameters. Will move m_initData to the resulting object.
     * This function creates a global variable whose type is a POINTER to the given type, because global variables are
     * pointers to their declared data type.
     */
    MirGlobalVar *build(MirGlobalVarLinkage linkage,
                        class MirType *type,
                        const std::pmr::string &name,
                        class SourceReference *sourceRef = nullptr);

    /**
     * Sets the constness of the future result object.
     */
    MirGlobalVarBuilder &setConstant(bool constant);

    /**
     * Adds an initializer to this variable. The initializer can only be a constant value: MirInteger, MirFloat or
     * MirConstantArray, in any other cases a diag error will be thrown.
     */
    MirGlobalVarBuilder &setInitializer(class MirOperand *initializer);

  private:
    bool m_constant; // True by default.
    MirBuilderContext *m_ctx;
    class MirOperand *m_initializer;
};

#endif // EZPACKER_MIRGLOBALVARBUILDER_H