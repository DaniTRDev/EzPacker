#ifndef EZPACKER_MIRGLOBALVARBUILDER_H
#define EZPACKER_MIRGLOBALVARBUILDER_H

#include "EzMirCommon.h"
#include "MirGlobalVar.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Printer/MirPrinter.h"

class MirGlobalVarBuilder : public MirBuilder<MirGlobalVar>
{
  public:
    /**
     * Creates the builder with the given context.
     */
    MirGlobalVarBuilder(MirBuilderContext *ctx);

    /**
     * Creates the global variable with the given parameters. Will move m_initData to the resulting object.
     * @param linkage
     * @param type
     * @param name
     * @param sourceRef
     */
    MirGlobalVar *build(MirGlobalVarLinkage linkage,
                        MirType *type,
                        const std::pmr::string &name,
                        SourceReference *sourceRef = nullptr);

    /**
     * Sets the constness of the future result object.
     * @param constant
     */
    MirGlobalVarBuilder &setConstant(bool constant);

    /**
     * Adds an initializer to this variable. The initializer can only be a constant value: MirInteger, MirFloat or MirConstantArray,
     * in any other cases a diag error will be thrown.
     * @param initializer
     */
    MirGlobalVarBuilder &setInitializer(MirOperand *initializer);

  private:
    bool m_constant; // True by default.
    MirBuilderContext *m_ctx;
    MirOperand *m_initializer;
};

#endif // EZPACKER_MIRGLOBALVARBUILDER_H