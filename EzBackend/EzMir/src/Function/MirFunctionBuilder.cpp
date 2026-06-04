#include "Function/MirFunctionBuilder.h"

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirFunctionBuilder::~MirFunctionBuilder() { MirBuilder::flush(); }

MirFunction *MirFunctionBuilder::build(MirType *returnType,
                                       SourceReference *sourceRef,
                                       const std::pmr::list<MirFuncParam *> &parameters,
                                       const std::pmr::string &name)
{
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator<MirFunction> funcAlloc(arena);
    std::pmr::polymorphic_allocator<MirFunctionStackFrame> funcStackFrameAlloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirFunctionStackFrame *stackFrame = funcStackFrameAlloc.allocate(1);
    MirFunction *func = funcAlloc.allocate(1);
    MirBlockBuilder builder(m_ctx, func);
    MirBlock *entryPoint = builder.build(sourceRef);
    std::pmr::list<MirBlock *> blocks(arena);

    blocks.push_back(entryPoint);
    funcAlloc.construct(stackFrame, std::pmr::vector<StackFrameObject *>(arena));
    funcAlloc.construct(func,
                        entryPoint,
                        stackFrame,
                        returnType,
                        m_ctx->createId(),
                        sourceRef,
                        blocks,
                        parameters,
                        name);

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirFunctionBuilder");
    diagBuilder << sourceRef << std::pmr::string(std::format("Built func with id: {}", func->getId()));
    diagBuilder.appendNote(std::pmr::string(MirPrinter().printToString(func)), sourceRef);

    if (!m_ctx->appendFunction(func))
    {
        return nullptr;
    }

    setBuildResult(func);
    return func;
}

MirBlockBuilder MirFunctionBuilder::blockBuilder() { return MirBlockBuilder(m_ctx, getBuiltObj()); }