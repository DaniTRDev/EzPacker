#include "Function/MirFunctionBuilder.h"

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) : m_ctx(ctx), m_parameters(ctx->getFuncAllocator()) {}

MirBlockBuilder MirFunctionBuilder::blockBuilder() { return MirBlockBuilder(m_ctx, &getBuiltObj()->getBlocks()); }

MirFunction *MirFunctionBuilder::build(MirType *returnType,
                                       const std::pmr::string &name,
                                       const std::pmr::list<MirRegister *> &parameters,
                                       SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator<MirFunction> funcAlloc(arena);
    std::pmr::polymorphic_allocator<MirFunctionStackFrame> funcStackFrameAlloc(arena);
    std::pmr::list<MirBlock *> blocks(arena);

    // InsertAfter given parameters to the ones already registered.
    m_parameters.insert(m_parameters.end(), parameters.begin(), parameters.end());

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirFunctionStackFrame *stackFrame =
            funcStackFrameAlloc.new_object<MirFunctionStackFrame>(std::pmr::vector<StackFrameObject *>(arena));

    MirBlockBuilder builder(m_ctx, &blocks);
    MirBlock *entryPoint = builder.build(sourceRef, "entryPoint");
    MirFunction *func = funcAlloc.new_object<MirFunction>(entryPoint,
                                                          stackFrame,
                                                          returnType,
                                                          m_ctx->createId(),
                                                          sourceRef,
                                                          blocks,
                                                          m_parameters,
                                                          name);

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirFunctionBuilder");
    diagBuilder << sourceRef << std::pmr::string(std::format("Built func with id: {}", func->getId()));
    diagBuilder.appendNote(std::pmr::string(MirPrinter().printToString(func, MirPrinterDetail::Detailed)), sourceRef);

    if (!m_ctx->appendFunction(func))
    {
        return nullptr;
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

StackFrameObject *MirFunctionBuilder::buildLocalStackObj(size_t size, size_t align)
{
    MirFunction *func = getBuiltObj();

    if (!getBuiltObj())
        return nullptr;

    StackFrameObject *obj = func->getStackFrame()->create(0, align, size, StackFrameObjectSource::Variable);
    SourceReference *ref = func->getSourceRef();

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirFunctionBuilder");
    diagBuilder << ref << std::pmr::string(std::format("%func.name={}.id={}", func->getName(), func->getId()));
    diagBuilder.appendNote(std::pmr::string(std::format("%lsto.id={}", obj->m_id)), nullptr);

    return obj;
}

StackFrameObject *MirFunctionBuilder::buildStackSpill(size_t size, size_t align)
{
    MirFunction *func = getBuiltObj();

    if (!getBuiltObj())
        return nullptr;

    StackFrameObject *obj = func->getStackFrame()->create(0, align, size, StackFrameObjectSource::Spill);
    SourceReference *ref = func->getSourceRef();

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirFunctionBuilder");
    diagBuilder << ref << std::pmr::string(std::format("%func.name={}.id={}", func->getName(), func->getId()));
    diagBuilder.appendNote(std::pmr::string(std::format("%lsts.id={}", obj->m_id)), nullptr);

    return obj;
}

StackFrameObject *MirFunctionBuilder::buildStackParam(size_t size, size_t align, int64_t offset)
{
    MirFunction *func = getBuiltObj();

    if (!getBuiltObj())
        return nullptr;

    StackFrameObject *obj = func->getStackFrame()->create(offset, align, size, StackFrameObjectSource::Parameter);
    SourceReference *ref = func->getSourceRef();

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirFunctionBuilder");
    diagBuilder << ref << std::pmr::string(std::format("%func.name={}.id={}", func->getName(), func->getId()));
    diagBuilder.appendNote(std::pmr::string(std::format("%lstp.id={}.offset={}", obj->m_id, obj->m_offset)), nullptr);

    return obj;
}
