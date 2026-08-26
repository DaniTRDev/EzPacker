#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "Printer/MirPrinter.h"

MirRegisterAllocatorPass::MirRegisterAllocatorPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_regAllocator(targetDesc->getRegisterAllocator()), m_result(ctx->getGlobalAllocator()),
    m_targetDesc(targetDesc)
{
}

const char *MirRegisterAllocatorPass::getName() const { return "RegisterAllocatorPass"; }

MirPassIterationPlace MirRegisterAllocatorPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult MirRegisterAllocatorPass::run(IntrusiveLinkedList<class MirFunction> &funcList,
                                            IntrusiveLinkedList<class MirFunction>::iterator it,
                                            class MirPassManager *passManager)
{
    bool allocationComplete = false;
    MirFunction *func = *it;
    size_t iterationCount = 0;
    constexpr size_t maxIterations = 100;

    std::pmr::polymorphic_allocator<> alloc(m_ctx->getGlobalAllocator());
    RegisterAllocatorCtx *ctx =
            alloc.new_object<RegisterAllocatorCtx>(m_ctx, func, m_targetDesc, m_ctx->getGlobalAllocator());

    auto cleanupFailure = [&]() -> MirPassResult
    {
        alloc.delete_object(ctx);
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    };

    while (!allocationComplete && iterationCount < maxIterations)
    {
        iterationCount++;

        // Reset per-iteration allocation state (preserve m_spilledRegs mapping across iterations)
        ctx->m_selectStack.clear();
        ctx->m_removedNodes.clear();
        ctx->m_allocatedRegs.clear();
        ctx->m_degree.clear();
        ctx->m_iGraph.clear();
        ctx->m_unspillableRegs.clear();

        LivenessAnalysisPass *livenessAnalysis = passManager->getAnalysis<LivenessAnalysisPass>(m_ctx);
        LivenessResult *result = livenessAnalysis->getResult();

        if (!m_regAllocator->buildInterferenceGraph(result, ctx))
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "RegisterAllocatorPass")
                    << "Failed to build interference graph for function " << func->getName();
            return cleanupFailure();
        }

        // Compute Initial Node Degrees & Lock Physical Nodes
        m_regAllocator->evaluateInterferenceGraphDegree(ctx);

        // Simplify Graph Nodes onto Select Stack
        if (!m_regAllocator->simplify(ctx))
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "RegisterAllocatorPass")
                    << "Simplification failed for function " << func->getName();
            return cleanupFailure();
        }

        // Assign Colors (or emit spills and rewrite IR)
        allocationComplete = m_regAllocator->selectColors(ctx);
        passManager->invalidateAnalysis();
    }

    if (!allocationComplete)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "RegisterAllocatorPass")
                << "Register allocation exceeded maximum iteration limit without converging for " << func->getName();
        return cleanupFailure();
    }

    // Rewrite Virtual Registers to Physical Registers in MIR
    m_regAllocator->rewriteColors(ctx);

    m_result.m_contexts[func] = ctx;
    m_result.m_resolvedFunctions.insert(func);

    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

const MirRegisterAllocatorPassResult &MirRegisterAllocatorPass::getResult() const { return m_result; }

void MirRegisterAllocatorPass::reset()
{
    std::pmr::polymorphic_allocator<> alloc(m_ctx->getGlobalAllocator());
    for (auto &[func, ctx] : m_result.m_contexts)
    {
        alloc.delete_object(ctx);
    }

    m_result.m_contexts.clear();
    m_result.m_resolvedFunctions.clear();
}

void MirRegisterAllocatorPass::printResult()
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirRegisterAllocatorPass");
    log << std::format("Printing function register allocation result").c_str();

    for (auto &func : m_result.m_resolvedFunctions)
    {
        std::string str = MirPrinter::printToString(func, MirPrinterDetail::Detailed);
        log.appendNote(func->getSourceRef(), "{}", str);
    }
}

std::vector<std::type_index> MirRegisterAllocatorPass::getDependencies() const
{
    // TODO: FIll with instruction selector pass.
    return {};
}