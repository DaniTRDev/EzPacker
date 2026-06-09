#ifndef EZPACKER_MIRFUNCTIONBUILDER_H
#define EZPACKER_MIRFUNCTIONBUILDER_H

#include "EzMirCommon.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Printer/MirPrinter.h"
#include "Operand/MirOperandBuilder.h"

class MirFunctionBuilder : public MirBuilder<MirFunction>
{
  public:
    /**
     * Craetes the function builder with the given context.
     * @param ctx
     */
    MirFunctionBuilder(MirBuilderContext *ctx);

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
                       const std::pmr::list<MirRegister *> &parameters,
                       const std::pmr::string &name);

    /**
     * Creates an abstract object in the function stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *buildLocalStackObj(size_t size, size_t align);

    /**
     * Creates an abstract object in the function stack frame.
     * @param size
     * @param align
     * @return
     */
    StackFrameObject *buildStackSpill(size_t size, size_t align);

    /**
     * Creates a parameter at the given offset. This is the only object whose offset is known at creation-time as this
     * is directly dictated by ABI.
     * @param size
     * @param align
     * @param offset
     * @return
     */
    StackFrameObject *buildStackParam(size_t size, size_t align, int64_t offset);

    /**
     * Adds a parameter into the FUTURE function that's going to be built.
     * @param type
     * @param sourceRef
     * @param name
     * @return
     */
    MirFunctionBuilder &buildParam(MirType *type, SourceReference *sourceRef, const std::pmr::string &name);

  private:
    MirBuilderContext *m_ctx;
    std::pmr::list<MirRegister *> m_parameters;
};

#endif // EZPACKER_MIRFUNCTIONBUILDER_H
