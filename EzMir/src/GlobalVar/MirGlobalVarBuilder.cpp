#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

MirGlobalVarBuilder::MirGlobalVarBuilder(MirBuilderContext *ctx) : m_ctx(ctx), m_initializer(nullptr)
{
    setConstant(true);
}

MirGlobalVarBuilder &MirGlobalVarBuilder::setConstant(bool constant)
{
    m_constant = constant;
    return *this;
}

MirGlobalVarBuilder &MirGlobalVarBuilder::setInitializer(MirOperand *initializer)
{
    if (!initializer->isOfType<MirInteger>() && !initializer->isOfType<MirFloat>())
    {
        auto diag = m_ctx->getDiagCollector()->builder(Diag_Error, "MirGlobalVarBuilder");
        diag << initializer->getSourceRef() << "Can't set a non-constant value to a global variable's initializer";
    }

    m_initializer = initializer;
    return *this;
}

MirGlobalVar *MirGlobalVarBuilder::build(MirGlobalVarLinkage linkage,
                                         MirType *type,
                                         const std::pmr::string &name,
                                         SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    MirGlobalVar *var = alloc.new_object<MirGlobalVar>(m_constant,
                                                       m_ctx->createId(),
                                                       linkage,
                                                       type,
                                                       m_initializer,
                                                       sourceRef,
                                                       name);

    auto diag = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirMirGlobalVarBuilder");
    diag << sourceRef << std::pmr::string(std::format("Built global var with id: {}", var->getId()));
    diag.appendNote(MirPrinter::printToString(var, MirPrinterDetail::Detailed).c_str(), nullptr);

    return var;
}