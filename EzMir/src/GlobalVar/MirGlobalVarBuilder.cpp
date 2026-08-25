#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

/**
 * Initializes the builder with the parent context and default constant setting (true).
 */
MirGlobalVarBuilder::MirGlobalVarBuilder(MirBuilderContext *ctx) : m_ctx(ctx), m_initializer(nullptr)
{
    setConstant(true);
}

/**
 * Configures the mutability / constancy of the global variable.
 */
MirGlobalVarBuilder &MirGlobalVarBuilder::setConstant(bool constant)
{
    m_constant = constant;
    return *this;
}

/**
 * Sets the initializer operand, validating that only constant values (integers or floats) are used.
 */
MirGlobalVarBuilder &MirGlobalVarBuilder::setInitializer(MirOperand *initializer)
{
    if (!initializer->isOfType<MirInteger>() && !initializer->isOfType<MirFloat>())
    {
        m_ctx->getDiagCollector()->error("MirGlobalVarBuilder",
                                         "Can't set a non-constant value to a global variable's initializer")
                << initializer->getSourceRef();
    }

    m_initializer = initializer;
    return *this;
}

/**
 * Constructs and allocates a new MirGlobalVar in the context arena allocator with a unique MIR ID.
 */
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

    auto diag =
            m_ctx->getDiagCollector()->trace("MirMirGlobalVarBuilder", "Built global var with id: {}", var->getId());
    diag << sourceRef;
    diag.appendNote(MirPrinter::printToString(var, MirPrinterDetail::Detailed));

    return var;
}