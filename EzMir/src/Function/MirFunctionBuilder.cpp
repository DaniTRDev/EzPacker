#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperandBuilder.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) :
    m_callingConv(nullptr), m_ctx(ctx), m_parameters(ctx->getGlobalAllocator()), m_owner(nullptr)
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
        m_ctx->getDiagCollector()->error("MirFunctionBuilder", "Can't create block builder from non-built function");
        return MirBlockBuilder(nullptr, static_cast<MirFunction *>(nullptr));
    }

    return MirBlockBuilder(m_ctx, obj);
}

MirFunction *MirFunctionBuilder::build(MirType *returnType, const std::pmr::string &name, SourceReference *sourceRef)
{
    const auto &t = m_ctx->getTypeTable();
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirFunction> funcAlloc(arena);
    std::pmr::polymorphic_allocator<MirFunctionStackFrame> funcStackFrameAlloc(arena);

    auto *stackFrame =
            funcStackFrameAlloc.new_object<MirFunctionStackFrame>(std::pmr::vector<StackFrameObject *>(arena));
    MirType *funcType = t->getFuncType(returnType, m_parameters, name);

    if (!m_callingConv)
    {
        m_callingConv = m_ctx->getDefaultCallingConvention();
    }

    MirFunction *func = funcAlloc.new_object<MirFunction>(m_callingConv,
                                                          nullptr,
                                                          stackFrame,
                                                          returnType,
                                                          funcType,
                                                          m_ctx->createId(),
                                                          sourceRef,
                                                          m_parameters,
                                                          name,
                                                          arena);

    MirBlockBuilder builder(m_ctx, func);
    MirBlock *entryPoint = builder.build(sourceRef, "entryPoint");

    entryPoint->setOwner(func);
    func->setEntryPoint(entryPoint);

    auto diagBuilder = m_ctx->getDiagCollector()->trace("MirFunctionBuilder", "Built func with id: {}", func->getId());
    diagBuilder.appendNote(sourceRef, MirPrinter::printToString(func, MirPrinterDetail::Detailed));
    diagBuilder.appendNote("Using calling convention: {}", m_callingConv->getName());

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

MirFunctionBuilder &MirFunctionBuilder::setCallingConvention(CallingConvDesc *cc)
{
    m_callingConv = cc;
    return *this;
}