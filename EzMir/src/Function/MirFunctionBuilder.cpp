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

/**
 * Creates a builder bound to a context.
 */
MirFunctionBuilder::MirFunctionBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

/**
 * Produces a block builder for the function built so far. Returns an invalid builder and emits an
 * error if the underlying function has not been produced yet.
 */
MirBlockBuilder MirFunctionBuilder::blockBuilder()
{
    MirFunction *obj = getBuiltObj();
    if (!obj)
    {
        m_ctx->getDiagCollector()->error("MirFunctionBuilder", "Can't create block builder from non-built function");
        // Keep the context so the returned builder is inert rather than crash-prone; callers can
        // detect the failure via MirBlockBuilder::isBuilt().
        return MirBlockBuilder(m_ctx, nullptr);
    }

    return MirBlockBuilder(m_ctx, obj);
}

/**
 * Appends a parameter to the end of the function's parameter list.
 */
MirFunctionBuilder &MirFunctionBuilder::addParam(MirFunction *func, MirRegister *param)
{
    func->m_parameters.push_back(param);
    return *this;
}

/**
 * Prepends a parameter to the front of the function's parameter list.
 */
MirFunctionBuilder &MirFunctionBuilder::addParamFront(MirFunction *func, MirRegister *param)
{
    func->m_parameters.push_front(param);
    return *this;
}

/**
 * Records that a physical register is used by the function, contributing to its callee-saved set.
 */
MirFunctionBuilder &MirFunctionBuilder::addPhysRegUse(MirFunction *func, const class MirRegisterRef &ref)
{
    if (std::find(func->m_usedCalleeSavedRegs.begin(), func->m_usedCalleeSavedRegs.end(), ref) ==
        func->m_usedCalleeSavedRegs.end())
    {
        func->m_usedCalleeSavedRegs.push_back(ref);
    }
    return *this;
}

/**
 * Allocates the function and its stack frame in the global arena, creates the entry block,
 * registers the parameter list and returns the built function (or nullptr if registration fails).
 */
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

    // Every function owns a stack frame describing its outgoing arguments and local objects.
    auto *stackFrame =
            funcStackFrameAlloc.new_object<MirFunctionStackFrame>(std::pmr::vector<StackFrameObject *>(arena));
    // Derive (or reuse) the function type describing the signature in the type table.
    MirType *funcType = t->getFuncType(returnType, parameters, name);

    // Fall back to the context's default calling convention when none was supplied.
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

    // Craft an initial entry block so every function begins with a valid basic block.
    MirBlockBuilder builder(m_ctx, func);
    MirBlock *entryPoint = builder.build(sourceRef, "entryPoint");

    func->setEntryPoint(entryPoint);
    // Seed the function's parameter list with the requested parameters.
    func->m_parameters.insert(func->m_parameters.begin(), parameters.begin(), parameters.end());

    // Only format the full function dump when the trace diagnostic is actually enabled.
    if (m_ctx->getDiagCollector()->isDiagEnabledForType(DiagnosticMessageType::Diag_Trace))
    {
        auto diagBuilder = m_ctx->getDiagCollector()->trace("MirFunctionBuilder", "Built func with id: {}", func->getId());
        diagBuilder.appendNote(sourceRef, MirPrinter::printToString(func, MirPrinterDetail::Detailed));
        diagBuilder.appendNote("Using calling convention: {}", cc->getName());
    }

    // Registration enforces global ID uniqueness; abort if it fails.
    if (!m_ctx->appendFunction(func))
    {
        return nullptr;
    }

    setBuildResult(func);
    return func;
}