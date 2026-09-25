#include "CompilationPipeline.h"
#include "DriverContext.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/NonSsaToSsaPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "MirPasses/Passes/MirPeepholePass.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizerPass.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Passes/MirTargetPeepholePass.h"
#include "MirPasses/MirPassManager.h"
#include <chrono>

namespace EzCompiler
{

namespace
{

/**
 * Walks every function/block/instruction once, delegating the textual representation to a
 * formatter. Shared by dumpCurrentMir() and dumpAssembly(), which previously duplicated the walk.
 */
template <typename Formatter>
std::string dumpFunctions(MirBuilderContext *bCtx, Formatter &formatter)
{
    std::string out;
    if (!bCtx)
    {
        return out;
    }

    for (MirFunction *func : bCtx->getFunctions())
    {
        if (!func)
        {
            continue;
        }
        formatter.beginFunction(out, func);
        for (MirBlock *block : func->getBlocks())
        {
            if (!block)
            {
                continue;
            }
            formatter.beginBlock(out, func, block);
            for (MirInstruction *inst : block->getInstructions())
            {
                if (!inst)
                {
                    continue;
                }
                formatter.instruction(out, inst);
            }
        }
        formatter.endFunction(out, func);
    }
    return out;
}

/// Formats the generic MIR dump (function/block/instruction with indentation).
struct MirDumpFormatter
{
    void beginFunction(std::string &out, const MirFunction *func) const
    {
        out += std::format("function @{}() {{\n", func->getName());
    }

    void beginBlock(std::string &out, const MirFunction *, const MirBlock *block) const
    {
        out += std::format("{}:\n", block->getName());
    }

    void instruction(std::string &out, const MirInstruction *inst) const { out += std::format("    {}\n", inst->toString()); }

    void endFunction(std::string &out, const MirFunction *) const { out += "}\n\n"; }
};

/// Formats the assembly-like listing (global labels, per-block labels, mnemonics).
struct AssemblyDumpFormatter
{
    void beginFunction(std::string &out, const MirFunction *func) const
    {
        out += std::format(".globl {}\n", func->getName());
        out += std::format("{}:\n", func->getName());
    }

    void beginBlock(std::string &out, const MirFunction *func, const MirBlock *block) const
    {
        out += std::format(".{}_{}:\n", func->getName(), block->getName());
    }

    void instruction(std::string &out, const MirInstruction *inst) const
    {
        if (inst->getTargetDesc())
        {
            out += std::format("    {}\n", inst->getTargetDesc()->getName());
        }
        else
        {
            out += std::format("    {}\n", inst->getOpCodeName());
        }
    }

    void endFunction(std::string &, const MirFunction *) const {}
};

} // namespace

CompilationPipeline::CompilationPipeline(DriverContext &ctx) : m_ctx(ctx) {}

bool CompilationPipeline::runPipeline()
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    if (!bCtx)
    {
        return false;
    }

    // Build one pass manager per stage (and register its analyses) once, instead of reconstructing
    // them for every function. invalidateAnalysis() is called per function to give each function the
    // same fresh-analysis state a newly constructed manager would have.
    MirPassManager middleEndManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    middleEndManager.setTestMode();
    middleEndManager.addPass<CodeFlowAnalysisPass>(bCtx);

    MirPassManager legalizationManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    legalizationManager.setTestMode();

    MirPassManager targetLoweringManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    targetLoweringManager.setTestMode();
    targetLoweringManager.addPass<CodeFlowAnalysisPass>(bCtx);
    targetLoweringManager.addPass<LivenessAnalysisPass>(bCtx);

    for (MirFunction *func : bCtx->getFunctions())
    {
        if (!func)
        {
            continue;
        }

        // 1. Middle-End Passes (CFG, SSA, Liveness)
        if (!runMiddleEndPasses(func, middleEndManager))
        {
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::GenericMir)
        {
            continue;
        }

        // 2. Legalization Passes (Signatures, Ops, Types)
        if (!runLegalizationPasses(func, legalizationManager))
        {
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::LegalizedMir)
        {
            continue;
        }

        // 3. Backend Target Lowering (ABI, ISel, RegAlloc, Frame)
        if (!runTargetLoweringPasses(func, targetLoweringManager))
        {
            return false;
        }
    }

    return true;
}

void CompilationPipeline::printPassRunning(std::string_view passName, const MirFunction *func) const
{
    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running " << passName << " on " << (func ? func->getName() : "") << "\n";
    }
}

bool CompilationPipeline::reportPassFailure(const char *message)
{
    m_ctx.getDiagCollector()->error("EzCompiler", "{}", message);
    return false;
}

template <typename PassT, typename... Args>
bool CompilationPipeline::runCheckedPass(std::string_view passName,
                                         MirFunction *func,
                                         MirPassManager *passManager,
                                         const char *failureMessage,
                                         Args &&...args)
{
    printPassRunning(passName, func);

    const auto start = std::chrono::steady_clock::now();
    PassT pass(std::forward<Args>(args)...);
    MirPassResult result = pass.run(m_ctx.getBuilderContext()->getFunctions().to_iterator(func), passManager);
    if (m_ctx.getOptions().timePasses)
    {
        const double elapsedMs =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::cout << "[Time] " << passName << ": " << elapsedMs << " ms\n";
    }

    if (!result.m_succeeded)
    {
        return reportPassFailure(failureMessage);
    }
    return true;
}

bool CompilationPipeline::runMiddleEndPasses(MirFunction *func, MirPassManager &passManager)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    passManager.invalidateAnalysis();

    // Materialize the CFG analysis so subsequent passes can retrieve it from the manager.
    printPassRunning("CodeFlowAnalysisPass", func);
    CodeFlowAnalysisPass *cfPass = passManager.getAnalysis<CodeFlowAnalysisPass>(bCtx);
    (void)cfPass;

    if (!runCheckedPass<NonSsaToSsaPass>(
                "NonSsaToSsaPass", func, &passManager, "SSA construction failed", bCtx))
    {
        return false;
    }

    if (!runCheckedPass<LivenessAnalysisPass>(
                "LivenessAnalysisPass", func, &passManager, "Liveness analysis failed", bCtx))
    {
        return false;
    }

    if (m_ctx.getOptions().optLevel != OptimizationLevel::O0)
    {
        if (!runCheckedPass<MirPeepholePass>(
                    "MirPeepholePass", func, &passManager, "Generic peephole optimization failed", bCtx))
        {
            return false;
        }
        passManager.invalidateAnalysis();
        if (!runCheckedPass<LivenessAnalysisPass>(
                    "LivenessAnalysisPass", func, &passManager, "Liveness analysis update failed", bCtx))
        {
            return false;
        }
    }

    return true;
}

bool CompilationPipeline::runLegalizationPasses(MirFunction *func, MirPassManager &passManager)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    TargetDesc *targetDesc = m_ctx.getTargetDesc();
    passManager.invalidateAnalysis();

    if (!runCheckedPass<MirFunctionSignatureLegalizerPass>(
                "MirFunctionSignatureLegalizerPass",
                func,
                &passManager,
                "Function signature legalization failed",
                bCtx,
                targetDesc))
    {
        return false;
    }

    if (!runCheckedPass<MirLegalizerPass>(
                "MirLegalizerPass", func, &passManager, "Operation legalization failed", bCtx, targetDesc))
    {
        return false;
    }

    return true;
}

bool CompilationPipeline::runTargetLoweringPasses(MirFunction *func, MirPassManager &passManager)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    TargetDesc *targetDesc = m_ctx.getTargetDesc();
    passManager.invalidateAnalysis();

    if (!runCheckedPass<MirAbiLowererPass>(
                "MirAbiLowererPass", func, &passManager, "ABI lowering failed", bCtx))
    {
        return false;
    }

    if (!runCheckedPass<MirInstructionSelectorPass>(
                "MirInstructionSelectorPass", func, &passManager, "Instruction selection failed", bCtx, targetDesc))
    {
        return false;
    }

    const bool enableCoalescing = (m_ctx.getOptions().optLevel != OptimizationLevel::O0);
    if (!runCheckedPass<MirRegisterAllocatorPass>(
                "MirRegisterAllocatorPass",
                func,
                &passManager,
                "Register allocation failed",
                bCtx,
                targetDesc,
                enableCoalescing))
    {
        return false;
    }

    if (!runCheckedPass<MirFrameLowererPass>(
                "MirFrameLowererPass", func, &passManager, "Frame lowering failed", bCtx, targetDesc))
    {
        return false;
    }

    if (m_ctx.getOptions().optLevel != OptimizationLevel::O0)
    {
        if (!runCheckedPass<MirTargetPeepholePass>(
                    "MirTargetPeepholePass", func, &passManager, "Target peephole optimization failed", bCtx, targetDesc))
        {
            return false;
        }
    }

    return true;
}

std::string CompilationPipeline::dumpCurrentMir() const
{
    MirDumpFormatter formatter;
    return dumpFunctions(m_ctx.getBuilderContext(), formatter);
}

std::string CompilationPipeline::dumpAssembly() const
{
    AssemblyDumpFormatter formatter;
    return dumpFunctions(m_ctx.getBuilderContext(), formatter);
}

} // namespace EzCompiler
