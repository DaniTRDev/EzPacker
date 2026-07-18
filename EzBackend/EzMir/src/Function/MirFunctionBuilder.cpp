#include "Function/MirFunctionBuilder.h"

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) :
    m_ctx(ctx), m_parameters(ctx->getFuncAllocator()), m_owner(nullptr)
{
}

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx, std::pmr::vector<MirFunction *> *owner) :
    MirFunctionBuilder(ctx)
{
    m_owner = owner;
}

MirBlockBuilder MirFunctionBuilder::blockBuilder()
{
    MirFunction *obj = getBuiltObj();
    if (!obj)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirFunctionBuilder")
                << "Can't create block builder from non-built function";
        return MirBlockBuilder(nullptr, (MirFunction *)nullptr); // Ambiguous call if cast is not set.
    }

    return MirBlockBuilder(m_ctx, obj->getBlocksPtr());
}

MirFunction *MirFunctionBuilder::build(MirType *returnType, const std::pmr::string &name, SourceReference *sourceRef)
{
    const auto &t = m_ctx->getTypeTable();
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator<MirFunction> funcAlloc(arena);
    std::pmr::polymorphic_allocator<MirFunctionStackFrame> funcStackFrameAlloc(arena);
    std::pmr::list<MirBlock *> blocks(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    auto stackFrame =
            funcStackFrameAlloc.new_object<MirFunctionStackFrame>(std::pmr::vector<StackFrameObject *>(arena));
    MirType *funcType = t->getFuncType(returnType, m_parameters, name);

    MirBlockBuilder builder(m_ctx, &blocks);
    MirBlock *entryPoint = builder.build(sourceRef, "entryPoint");
    MirFunction *func = funcAlloc.new_object<MirFunction>(entryPoint,
                                                          stackFrame,
                                                          returnType,
                                                          funcType,
                                                          m_ctx->createId(),
                                                          sourceRef,
                                                          blocks,
                                                          m_parameters,
                                                          name);

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirFunctionBuilder");
    diagBuilder << std::pmr::string(std::format("Built func with id: {}", func->getId()));
    diagBuilder.appendNote(std::pmr::string(MirPrinter::printToString(func, MirPrinterDetail::Detailed)), sourceRef);

    if (!m_ctx->appendFunction(func))
    {
        return nullptr;
    }

    if (m_owner)
    {
        m_owner->push_back(func);
    }

    setBuildResult(func);
    return func;
}

MirFunctionBuilder &
MirFunctionBuilder::buildParam(MirType *type, const std::pmr::string &name, SourceReference *sourceRef)
{
    MirOperandBuilder builder(m_ctx);
    m_parameters.push_back(builder.buildVReg(type, name, sourceRef));

    return *this;
}

MirFunctionBuilder &MirFunctionBuilder::buildParam(MirRegister *param)
{
    m_parameters.push_back(param);
    return *this;
}