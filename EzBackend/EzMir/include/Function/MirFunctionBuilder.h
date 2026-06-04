#ifndef EZPACKER_MIRFUNCTIONBUILDER_H
#define EZPACKER_MIRFUNCTIONBUILDER_H

#include "EzCoreCommon.h"
#include "Builder/MirBuilder.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Printer/MirPrinter.h"

class MirFunctionBuilder : public MirBuilder<MirFunction>
{
  public:
    /**
     * Craetes the function builder with the given context.
     * @param ctx
     */
    MirFunctionBuilder(MirBuilderContext *ctx);

    /**
     * Flushes the function and appends it to the context.
     */
    ~MirFunctionBuilder() override;

    /**
     * Returns a block builder attached to the current function.
     * @param sourceRef
     * @return
     */
    MirBlockBuilder blockBuilder();

    /**
     * Builds a function over arena-managed MIR data structures.
     *
     * @param returnType    MIR type describing the function's return value.
     * @param parameters    Slice of parameter operands in declaration order.
     * @param name
     */
    MirFunction *build(MirType *returnType,
                       SourceReference *sourceRef,
                       const std::pmr::list<MirFuncParam *> &parameters,
                       const std::pmr::string &name);

    /**
     * Returns an instruction builder linked to end of the latest block of the function.
     * @return
     */
    MirInstructionBuilder instrBuilder();

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_MIRFUNCTIONBUILDER_H
