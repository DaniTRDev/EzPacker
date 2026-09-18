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
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizerPass.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "MirPasses/MirPassManager.h"
#include <sstream>

namespace EzCompiler
{

CompilationPipeline::CompilationPipeline(DriverContext &ctx) :
    m_ctx(ctx)
{
}

bool CompilationPipeline::runPipeline()
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    if (!bCtx)
    {
        return false;
    }

    for (MirFunction *func : bCtx->getFunctions())
    {
        if (!func)
        {
            continue;
        }

        // 1. Middle-End Passes (CFG, SSA, Liveness)
        if (!runMiddleEndPasses(func))
        {
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::GenericMir)
        {
            continue;
        }

        // 2. Legalization Passes (Signatures, Ops, Types)
        if (!runLegalizationPasses(func))
        {
            return false;
        }

        if (m_ctx.getOptions().emissionStage == EmissionStage::LegalizedMir)
        {
            continue;
        }

        // 3. Backend Target Lowering (ABI, ISel, RegAlloc, Frame)
        if (!runTargetLoweringPasses(func))
        {
            return false;
        }
    }

    return true;
}

bool CompilationPipeline::runMiddleEndPasses(MirFunction *func)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    auto it = bCtx->getFunctions().to_iterator(func);
    MirPassManager passManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    passManager.setTestMode();
    passManager.addPass<CodeFlowAnalysisPass>(bCtx);

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running CodeFlowAnalysisPass on " << func->getName() << "\n";
    }
    CodeFlowAnalysisPass *cfPass = passManager.getAnalysis<CodeFlowAnalysisPass>(bCtx);
    (void)cfPass;

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running NonSsaToSsaPass on " << func->getName() << "\n";
    }
    NonSsaToSsaPass ssaPass(bCtx);
    ssaPass.run(it, &passManager);

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running LivenessAnalysisPass on " << func->getName() << "\n";
    }
    LivenessAnalysisPass livePass(bCtx);
    livePass.run(it, &passManager);

    return true;
}

bool CompilationPipeline::runLegalizationPasses(MirFunction *func)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    TargetDesc *targetDesc = m_ctx.getTargetDesc();
    auto it = bCtx->getFunctions().to_iterator(func);
    MirPassManager passManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    passManager.setTestMode();

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirFunctionSignatureLegalizerPass on " << func->getName() << "\n";
    }
    MirFunctionSignatureLegalizerPass sigPass(bCtx, targetDesc);
    auto sigRes = sigPass.run(it, &passManager);
    if (!sigRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Function signature legalization failed");
        return false;
    }

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirLegalizerPass on " << func->getName() << "\n";
    }
    MirLegalizerPass legPass(bCtx, targetDesc);
    auto legRes = legPass.run(it, &passManager);
    if (!legRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Operation legalization failed");
        return false;
    }

    return true;
}

bool CompilationPipeline::runTargetLoweringPasses(MirFunction *func)
{
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    TargetDesc *targetDesc = m_ctx.getTargetDesc();
    auto it = bCtx->getFunctions().to_iterator(func);
    MirPassManager passManager(m_ctx.getDiagCollector(), m_ctx.getSessionAllocator());
    passManager.setTestMode();
    passManager.addPass<CodeFlowAnalysisPass>(bCtx);
    passManager.addPass<LivenessAnalysisPass>(bCtx);

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirAbiLowererPass on " << func->getName() << "\n";
    }
    MirAbiLowererPass abiPass(bCtx);
    auto abiRes = abiPass.run(it, &passManager);
    if (!abiRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "ABI lowering failed");
        return false;
    }

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirInstructionSelectorPass on " << func->getName() << "\n";
    }
    MirInstructionSelectorPass iselPass(bCtx, targetDesc);
    auto iselRes = iselPass.run(it, &passManager);
    if (!iselRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Instruction selection failed");
        return false;
    }

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirRegisterAllocatorPass on " << func->getName() << "\n";
    }
    MirRegisterAllocatorPass regAllocPass(bCtx, targetDesc);
    auto regRes = regAllocPass.run(it, &passManager);
    if (!regRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Register allocation failed");
        return false;
    }

    if (m_ctx.getOptions().printPasses)
    {
        std::cout << "[Pass] Running MirFrameLowererPass on " << func->getName() << "\n";
    }
    MirFrameLowererPass framePass(bCtx, targetDesc);
    auto frameRes = framePass.run(it, &passManager);
    if (!frameRes.m_succeeded)
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Frame lowering failed");
        return false;
    }

    return true;
}

std::string CompilationPipeline::dumpCurrentMir() const
{
    std::ostringstream oss;
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    if (!bCtx) return {};

    for (MirFunction *func : bCtx->getFunctions())
    {
        if (!func) continue;
        oss << "function @" << func->getName() << "() {\n";
        for (MirBlock *block : func->getBlocks())
        {
            if (!block) continue;
            oss << block->getName() << ":\n";
            for (MirInstruction *inst : block->getInstructions())
            {
                if (!inst) continue;
                oss << "    " << inst->toString() << "\n";
            }
        }
        oss << "}\n\n";
    }
    return oss.str();
}

std::string CompilationPipeline::dumpAssembly() const
{
    std::ostringstream oss;
    MirBuilderContext *bCtx = m_ctx.getBuilderContext();
    if (!bCtx) return {};

    for (MirFunction *func : bCtx->getFunctions())
    {
        if (!func) continue;
        oss << ".globl " << func->getName() << "\n";
        oss << func->getName() << ":\n";
        for (MirBlock *block : func->getBlocks())
        {
            if (!block) continue;
            oss << "." << func->getName() << "_" << block->getName() << ":\n";
            for (MirInstruction *inst : block->getInstructions())
            {
                if (!inst) continue;
                if (inst->getTargetDesc())
                {
                    oss << "    " << inst->getTargetDesc()->getName();
                }
                else
                {
                    oss << "    " << inst->getOpCodeName();
                }
                oss << "\n";
            }
        }
    }
    return oss.str();
}

} // namespace EzCompiler
