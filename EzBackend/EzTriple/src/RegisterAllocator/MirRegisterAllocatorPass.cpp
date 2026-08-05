#include "RegisterAllocator/MirRegisterAllocatorPass.h"

MirRegisterAllocatorPass::MirRegisterAllocatorPass(MirBuilderContext *ctx,
                                                   MirRegisterAllocator *regAllocator,
                                                   TargetDesc *targetDesc) :
    m_ctx(ctx), m_regAllocator(regAllocator), m_targetDesc(targetDesc), m_resolvedFunctions(ctx->getGlobalAllocator())
{
}

const char *MirRegisterAllocatorPass::getName() const { return "RegisterAllocatorPass"; }

MirPassIterationPlace MirRegisterAllocatorPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult MirRegisterAllocatorPass::run(std::pmr::list<class MirFunction *> &funcList,
                                            std::pmr::list<class MirFunction *>::iterator it,
                                            class MirPassManager *passManager)
{
    bool allocationComplete = false;
    MirFunction *func = *it;
    RegisterAllocatorCtx ctx(m_ctx, func, m_targetDesc, m_ctx->getGlobalAllocator());
    size_t iterationCount = 0;
    constexpr size_t maxIterations = 100; // Safeguard against infinite allocation loops

    while (!allocationComplete && iterationCount < maxIterations)
    {
        iterationCount++;

        // Reset per-iteration allocation state (preserve m_spilledRegs mapping across iterations)
        ctx.m_selectStack.clear();
        ctx.m_removedNodes.clear();
        ctx.m_allocatedRegs.clear();
        ctx.m_degree.clear();
        ctx.m_iGraph.clear();

        LivenessAnalysisPass *livenessAnalysis = passManager->getAnalysis<LivenessAnalysisPass>(m_ctx);
        LivenessResult result = livenessAnalysis->getResult();

        if (!m_regAllocator->buildInterferenceGraph(&result, ctx))
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "RegisterAllocatorPass")
                    << "Failed to build interference graph for function " << func->getName();
            return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
        }

        // Compute Initial Node Degrees & Lock Physical Nodes
        m_regAllocator->evaluateInterferenceGraphDegree(ctx);

        // Simplify Graph Nodes onto Select Stack
        m_regAllocator->simplify(ctx);

        // Assign Colors (or emit spills and rewrite IR)
        allocationComplete = m_regAllocator->selectColors(ctx);
        passManager->invalidateAnalysis();
    }

    if (!allocationComplete)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "RegisterAllocatorPass")
                << "Register allocation exceeded maximum iteration limit without converging for " << func->getName();
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    // Rewrite Virtual Registers to Physical Registers in MIR
    m_regAllocator->rewriteColors(ctx);
    m_resolvedFunctions.push_back(func);

    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

void MirRegisterAllocatorPass::printResult() const
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirRegisterAllocatorPass");
    log << std::format("Printing function register allocation result").c_str();

    for (auto &func : m_resolvedFunctions)
    {
        std::string str = MirPrinter::printToString(func, MirPrinterDetail::Detailed);
        log.appendNote(str.c_str(), func->getSourceRef());
    }
}

std::vector<std::type_index> MirRegisterAllocatorPass::getDependencies() const
{
    return { std::type_index(typeid(MirInstructionSelectorPass)) };
}