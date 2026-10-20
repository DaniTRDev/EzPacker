#include "CompilationPipeline.h"
#include "DriverContext.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "MirPasses/MirPass.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/NonSsaToSsaPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "MirPasses/Passes/MirPeepholePass.h"
#include "MirPasses/Passes/MirVerifierPass.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizerPass.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Passes/MirTargetPeepholePass.h"
#include "MirPasses/MirPassManager.h"
#include "Printer/MirPrinter.h"
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

    for (const MirFunction *func : bCtx->getFunctions())
    {
        if (!func)
        {
            continue;
        }

        formatter.beginFunction(out, func);

        for (const MirBlock *block : func->getBlocks())
        {
            formatter.beginBlock(out, func, block);

            for (const MirInstruction *inst : block->getInstructions())
            {
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
        if (func && func->isDeclaration())
        {
            out += std::format("declare @{}();\n\n", func->getName());
            return;
        }
        out += std::format("function @{}() {{\n", func->getName());
    }

    void beginBlock(std::string &out, const MirFunction *, const MirBlock *block) const
    {
        out += std::format("{}:\n", block->getName());
    }

    void instruction(std::string &out, const MirInstruction *inst) const { out += std::format("    {}\n", inst->toString()); }

    void endFunction(std::string &out, const MirFunction *func) const
    {
        if (func && func->isDeclaration())
        {
            return;
        }
        out += "}\n\n";
    }
};

/// Formats the assembly-like listing (global labels, per-block labels, mnemonics).
struct AssemblyDumpFormatter
{
    void beginFunction(std::string &out, const MirFunction *func) const
    {
        if (func && func->isDeclaration())
        {
            out += std::format(".extern {}\n\n", func->getName());
            return;
        }
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

CompilationPipeline::CompilationPipeline(DriverContext &ctx) : m_ctx(ctx)
{
    if (!m_ctx.getOptions().pipelineDumpJsonPath.empty())
    {
        m_telemetry = std::make_unique<PipelineTelemetry>();
        m_telemetry->setTarget(m_ctx.getOptions().target.toString());
        m_telemetry->setInputFile(m_ctx.getOptions().inputFilePath);

        std::string optStr = "O0";
        switch (m_ctx.getOptions().optLevel)
        {
        case OptimizationLevel::O0: optStr = "O0"; break;
        case OptimizationLevel::O1: optStr = "O1"; break;
        case OptimizationLevel::O2: optStr = "O2"; break;
        case OptimizationLevel::Os: optStr = "Os"; break;
        }
        m_telemetry->setOptLevel(optStr);
        m_telemetry->bindSourceManager(m_ctx.getSourceManager());

        if (m_ctx.getDiagCollector())
        {
            m_ctx.getDiagCollector()->addListener(m_telemetry.get());
        }
    }
}

void CompilationPipeline::writeTelemetryIfRequested()
{
    if (m_telemetry && !m_ctx.getOptions().pipelineDumpJsonPath.empty())
    {
        m_telemetry->setAssembly(dumpAssembly());
        m_telemetry->writeToFile(m_ctx.getOptions().pipelineDumpJsonPath);
    }
}

bool CompilationPipeline::runPipeline()
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    if (!bCtx)
    {
        writeTelemetryIfRequested();
        return false;
    }

    if (m_telemetry)
    {
        m_telemetry->setInitialMir(dumpCurrentMir());
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
        if (!func || func->isDeclaration())
        {
            continue;
        }

        // 0. Input MIR Verification Pass (verifies SizeMatch, operand types, invariants before middle-end)
        if (!runInputVerification(func, middleEndManager))
        {
            writeTelemetryIfRequested();
            return false;
        }

        // 1. Middle-End Passes (CFG, SSA, Liveness)
        if (!runMiddleEndPasses(func, middleEndManager))
        {
            writeTelemetryIfRequested();
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::GenericMir)
        {
            continue;
        }

        // 2. Legalization Passes (Signatures, Ops, Types)
        if (!runLegalizationPasses(func, legalizationManager))
        {
            writeTelemetryIfRequested();
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::LegalizedMir)
        {
            continue;
        }

        // 3. Backend Target Lowering (ABI, ISel, RegAlloc, Frame)
        if (!runTargetLoweringPasses(func, targetLoweringManager))
        {
            writeTelemetryIfRequested();
            return false;
        }
    }

    writeTelemetryIfRequested();
    return true;
}

void CompilationPipeline::printPassRunning(std::string_view passName, const MirFunction *func) const
{
    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running " << passName << " on " << (func ? func->getName() : "") << std::endl;
    }
}

bool CompilationPipeline::reportPassFailure(const char *message)
{
    m_ctx.getDiagCollector()->error("EzCompiler", "{}", message);
    return false;
}

void CompilationPipeline::recordPassTelemetry(std::string_view passName,
                                              std::string_view stage,
                                              MirFunction *func,
                                              MirPassManager *passManager,
                                              const ::MirPassResult &result,
                                              double durationMs,
                                              CodeFlowAnalysisPass *cfPass,
                                              LivenessAnalysisPass *lvPass)
{
    if (!m_telemetry)
    {
        return;
    }

    PassTelemetryRecord rec;
    rec.index = m_passCounter++;
    rec.passName = std::string(passName);
    rec.stage = std::string(stage);
    rec.functionName = func ? std::string(func->getName()) : "";
    rec.passType = result.m_modifiedMir ? "Transform" : "Analysis";
    rec.durationMs = durationMs;
    rec.modifiedMir = result.m_modifiedMir;
    rec.blockCount = func ? func->getBlockCount() : 0;

    size_t instCount = 0;
    if (func)
    {
        for (const MirBlock *b : func->getBlocks())
        {
            instCount += b->getInstructions().size();
        }
    }
    rec.instructionCount = instCount;
    rec.mirDump = dumpCurrentMir();

    m_telemetry->recordPass(std::move(rec));

    if (func)
    {
        captureFunctionTelemetry(func, passManager, cfPass, lvPass);
    }
}

void CompilationPipeline::captureFunctionTelemetry(MirFunction *func,
                                                       MirPassManager *passManager,
                                                       CodeFlowAnalysisPass *cfPass,
                                                       LivenessAnalysisPass *lvPass)
{
    if (!func || !m_telemetry)
    {
        return;
    }

    FunctionTelemetry ft;
    ft.name = std::string(func->getName());

    if (!cfPass && passManager)
    {
        cfPass = passManager->getCachedAnalysis<CodeFlowAnalysisPass>();
    }
    if (!lvPass && passManager)
    {
        lvPass = passManager->getCachedAnalysis<LivenessAnalysisPass>();
    }

    for (const MirBlock *block : func->getBlocks())
    {
        CfgBlockTelemetry blk;
        blk.id = block->getId();
        blk.name = std::string(block->getName());
        blk.instructionCount = block->getInstructions().size();

        for (const MirInstruction *inst : block->getInstructions())
        {
            blk.instructions.push_back(inst->toString());
        }

        if (cfPass && cfPass->getResult())
        {
            const CodeFlowResult *cfg = cfPass->getResult();
            for (MirId predId : cfg->getPredecessors(block->getId()))
            {
                MirBlock *predBlk = func->getBlock(predId);
                if (predBlk)
                {
                    blk.predecessors.push_back(std::string(predBlk->getName()));
                }
                else
                {
                    blk.predecessors.push_back(std::format("%blk{}", predId));
                }
            }
            for (MirId succId : cfg->getSuccessors(block->getId()))
            {
                MirBlock *succBlk = func->getBlock(succId);
                if (succBlk)
                {
                    blk.successors.push_back(std::string(succBlk->getName()));
                }
                else
                {
                    blk.successors.push_back(std::format("%blk{}", succId));
                }
            }
        }

        ft.blocks.push_back(std::move(blk));

        if (lvPass && lvPass->getResult())
        {
            LivenessResult *lres = lvPass->getResult();
            LivenessBlockTelemetry lbt;
            lbt.blockId = block->getId();
            lbt.blockName = std::string(block->getName());

            auto fillRegSet = [](const std::pmr::unordered_set<MirRegisterRef> &src, std::vector<std::string> &dst) {
                for (const auto &ref : src)
                {
                    dst.push_back(MirPrinter::printToString(ref));
                }
            };

            auto itDef = lres->m_def.find(block->getId());
            if (itDef != lres->m_def.end()) fillRegSet(itDef->second, lbt.def);

            auto itUse = lres->m_use.find(block->getId());
            if (itUse != lres->m_use.end()) fillRegSet(itUse->second, lbt.use);

            auto itLiveIn = lres->m_liveIn.find(block->getId());
            if (itLiveIn != lres->m_liveIn.end()) fillRegSet(itLiveIn->second, lbt.liveIn);

            auto itLiveOut = lres->m_liveOut.find(block->getId());
            if (itLiveOut != lres->m_liveOut.end()) fillRegSet(itLiveOut->second, lbt.liveOut);

            ft.liveness.push_back(std::move(lbt));
        }
    }

    if (func->getAnalysisData())
    {
        ft.frameLayout.isComputed = true;
        ft.frameLayout.calleeSaveSize = func->getAnalysisData()->m_calleeSavedAreaSize;
        ft.frameLayout.totalSize = func->getAnalysisData()->m_totalFrameSize;
        ft.frameLayout.hasCalls = func->getAnalysisData()->m_hasCalls;
        ft.frameLayout.hasDynamicAllocs = func->getAnalysisData()->m_hasDynamicAllocs;
        for (const auto &regRef : func->getUsedCalleeSavedRegs())
        {
            ft.frameLayout.usedCalleeSavedRegs.push_back(MirPrinter::printToString(regRef));
        }
    }

    m_telemetry->updateFunctionAnalysis(std::string(func->getName()), std::move(ft));
}

template <typename PassT, typename... Args>
bool CompilationPipeline::runCheckedPass(std::string_view passName,
                                         std::string_view stage,
                                         MirFunction *func,
                                         MirPassManager *passManager,
                                         const char *failureMessage,
                                         Args &&...args)
{
    printPassRunning(passName, func);

    const auto start = std::chrono::steady_clock::now();
    PassT pass(std::forward<Args>(args)...);
    ::MirPassResult result = pass.run(m_ctx.getBuilderContext()->getFunctions().to_iterator(func), passManager);
    const double elapsedMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (m_ctx.getOptions().timePasses)
    {
        std::cout << "[Time] " << passName << ": " << elapsedMs << " ms\n";
    }

    CodeFlowAnalysisPass *cfPass = nullptr;
    LivenessAnalysisPass *lvPass = nullptr;
    if constexpr (std::is_same_v<PassT, CodeFlowAnalysisPass>)
    {
        cfPass = &pass;
    }
    if constexpr (std::is_same_v<PassT, LivenessAnalysisPass>)
    {
        lvPass = &pass;
    }

    if (m_telemetry)
    {
        recordPassTelemetry(passName, stage, func, passManager, result, elapsedMs, cfPass, lvPass);
    }

    if (!result.m_succeeded)
    {
        return reportPassFailure(failureMessage);
    }
    return true;
}

bool CompilationPipeline::runInputVerification(MirFunction *func, MirPassManager &passManager)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    return runCheckedPass<MirVerifierPass>(
            "MirVerifierPass", "Verification", func, &passManager, "Input MIR verification failed", bCtx);
}

bool CompilationPipeline::runMiddleEndPasses(MirFunction *func, MirPassManager &passManager)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    passManager.invalidateAnalysis();

    // Materialize the CFG analysis so subsequent passes can retrieve it from the manager.
    printPassRunning("CodeFlowAnalysisPass", func);
    const auto cfStart = std::chrono::steady_clock::now();
    CodeFlowAnalysisPass *cfPass = passManager.getAnalysis<CodeFlowAnalysisPass>(bCtx);
    (void)cfPass;
    const double cfElapsedMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cfStart).count();
    if (m_ctx.getOptions().timePasses)
    {
        std::cout << "[Time] CodeFlowAnalysisPass: " << cfElapsedMs << " ms\n";
    }
    if (m_telemetry)
    {
        ::MirPassResult cfRes{ false, true, true };
        recordPassTelemetry("CodeFlowAnalysisPass", "MiddleEnd", func, &passManager, cfRes, cfElapsedMs);
    }

    if (!runCheckedPass<NonSsaToSsaPass>(
                "NonSsaToSsaPass", "MiddleEnd", func, &passManager, "SSA construction failed", bCtx))
    {
        return false;
    }

    if (!runCheckedPass<LivenessAnalysisPass>(
                "LivenessAnalysisPass", "MiddleEnd", func, &passManager, "Liveness analysis failed", bCtx))
    {
        return false;
    }

    if (m_ctx.getOptions().optLevel != OptimizationLevel::O0)
    {
        if (!runCheckedPass<MirPeepholePass>(
                    "MirPeepholePass", "MiddleEnd", func, &passManager, "Generic peephole optimization failed", bCtx))
        {
            return false;
        }
        passManager.invalidateAnalysis();
        if (!runCheckedPass<LivenessAnalysisPass>(
                    "LivenessAnalysisPass", "MiddleEnd", func, &passManager, "Liveness analysis update failed", bCtx))
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
                "Legalization",
                func,
                &passManager,
                "Function signature legalization failed",
                bCtx,
                targetDesc))
    {
        return false;
    }

    if (!runCheckedPass<MirLegalizerPass>(
                "MirLegalizerPass", "Legalization", func, &passManager, "Operation legalization failed", bCtx, targetDesc))
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
                "MirAbiLowererPass", "TargetLowering", func, &passManager, "ABI lowering failed", bCtx))
    {
        return false;
    }

    if (!runCheckedPass<MirInstructionSelectorPass>(
                "MirInstructionSelectorPass", "TargetLowering", func, &passManager, "Instruction selection failed", bCtx, targetDesc))
    {
        return false;
    }

    const bool enableCoalescing = (m_ctx.getOptions().optLevel != OptimizationLevel::O0);
    if (!runCheckedPass<MirRegisterAllocatorPass>(
                "MirRegisterAllocatorPass",
                "TargetLowering",
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
                "MirFrameLowererPass", "TargetLowering", func, &passManager, "Frame lowering failed", bCtx, targetDesc))
    {
        return false;
    }

    if (m_ctx.getOptions().optLevel != OptimizationLevel::O0)
    {
        if (!runCheckedPass<MirTargetPeepholePass>(
                    "MirTargetPeepholePass", "TargetLowering", func, &passManager, "Target peephole optimization failed", bCtx, targetDesc))
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
