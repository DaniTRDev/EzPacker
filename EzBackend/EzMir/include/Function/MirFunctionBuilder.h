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
     * Creates the function builder with the given context.
     * @param ctx
     */
    MirFunctionBuilder(MirBuilderContext *ctx);

    /**
     * Creates the function builder linked to an owner vector container.
     * @param ctx
     */
    MirFunctionBuilder(MirBuilderContext *ctx, std::pmr::vector<MirFunction *> *owner);

    /**
     * Returns a block builder attached to the current function. If this function HAS NOT been built, an invalid
     * block builder is returned and a diagnostic error is pushed.
     * @param sourceRef
     * @return
     */
    MirBlockBuilder blockBuilder();

    /**
     * Builds a function over arena-managed MIR data structures.
     *
     * @param returnType    MIR type describing the function's return value.
     * @param name
     * @param sourceRef
     */
    MirFunction *build(MirType *returnType, const std::pmr::string &name = "", SourceReference *sourceRef = nullptr);

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
     * Creates a parameter at the given offset in the function stack frame. This is the only object whose offset is
     * known at creation-time.
     * @param size
     * @param align
     * @param offset
     * @return
     */
    StackFrameObject *buildStackParam(size_t size, size_t align, int64_t offset);

    /**
     * Adds a parameter into the FUTURE function that's going to be built. This does not affect the stack frame.
     * @param type
     * @param name
     * @param sourceRef
     * @return
     */
    MirFunctionBuilder &
    buildParam(MirType *type, const std::pmr::string &name = "", SourceReference *sourceRef = nullptr);

    /**
     * Appends the given already-built parameter into the function.
     * @param param
     */
    MirFunctionBuilder &buildParam(MirRegister *param);

  private:
    MirBuilderContext *m_ctx;
    std::pmr::list<MirRegister *> m_parameters;
    std::pmr::vector<MirFunction *> *m_owner;
};

#endif // EZPACKER_MIRFUNCTIONBUILDER_H
