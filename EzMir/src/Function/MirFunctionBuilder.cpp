#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirRegisterReference.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) : m_ctx(ctx), m_owner(nullptr) {}

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

MirFunctionBuilder &MirFunctionBuilder::addParam(MirFunction *func, MirRegister *param)
{
    func->m_parameters.push_back(param);
    return *this;
}

MirFunctionBuilder &MirFunctionBuilder::addParamFront(MirFunction *func, MirRegister *param)
{
    func->m_parameters.push_front(param);
    return *this;
}

MirFunctionBuilder &MirFunctionBuilder::addPhysRegUse(MirFunction *func, const class MirRegisterRef &ref)
{
    func->m_usedCalleeSavedRegs.push_back(ref);
    return *this;
}

MirFunction *MirFunctionBuilder::build(class MirType *returnType,
                                       std::initializer_list<MirRegister *> parameters,
                                       const std::string_view &name,
                                       class CallingConvDesc *cc,
                                       class SourceReference *sourceRef)
{
    const auto &t = m_ctx->getTypeTable();
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirFunction> funcAlloc(arena);
    std::pmr::polymorphic_allocator<MirFunctionStackFrame> funcStackFrameAlloc(arena);

    auto *stackFrame =
            funcStackFrameAlloc.new_object<MirFunctionStackFrame>(std::pmr::vector<StackFrameObject *>(arena));
    MirType *funcType = t->getFuncType(returnType, parameters, name);

    if (!cc)
    {
        cc = m_ctx->getDefaultCallingConvention();
    }

    std::pmr::string pmrName(name, arena);
    MirFunction *func = funcAlloc.new_object<MirFunction>(cc,
                                                          stackFrame,
                                                          returnType,
                                                          funcType,
                                                          m_ctx->createId(),
                                                          sourceRef,
                                                          std::move(pmrName),
                                                          arena);

    MirBlockBuilder builder(m_ctx, func);
    MirBlock *entryPoint = builder.build(sourceRef, "entryPoint");

    func->setEntryPoint(entryPoint);
    func->m_parameters.insert(func->m_parameters.begin(), parameters.begin(), parameters.end());

    auto diagBuilder = m_ctx->getDiagCollector()->trace("MirFunctionBuilder", "Built func with id: {}", func->getId());
    diagBuilder.appendNote(sourceRef, MirPrinter::printToString(func, MirPrinterDetail::Detailed));
    diagBuilder.appendNote("Using calling convention: {}", cc->getName());

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