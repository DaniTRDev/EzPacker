#include "MirPasses/MirPassManager.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"

MirPassManager::MirPassManager(DiagnosticCollector *diagCollector, std::pmr::memory_resource *globalArena) :
    m_testMode(false), m_diagCollector(diagCollector), m_validAnalyses(globalArena), m_passesBlueprint(globalArena),
    m_savedResults(globalArena), m_executionPipeline(globalArena)
{
}

DiagnosticCollector *MirPassManager::getDiagCollector() const { return m_diagCollector; }

MirPassResult MirPassManager::runPass(MirPass *pass, MirBuilderContext *ctx)
{
    m_diagCollector->trace("MirPassManager", "Running pass {}", pass->getName());
    pass->reset(); // Resets pass' previous results.

    auto &functionList = ctx->getFunctions();
    auto &globalList = ctx->getGlobalVars();
    MirPassResult combinedResult{ .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };

    switch (pass->getIterationPlace())
    {
        case MirPassIterationPlace::GlobalVariable:
        {
            for (auto *globalVar : globalList)
            {
                MirPassResult r = pass->run(globalVar, this);
                combinedResult.m_modifiedMir |= r.m_modifiedMir;
                if (!r.m_succeeded)
                {
                    combinedResult.m_succeeded = false;
                    break;
                }
            }
            break;
        }
        case MirPassIterationPlace::Function:
        {
            for (auto it = functionList.begin(); it != functionList.end(); ++it)
            {
                MirPassResult r = pass->run(functionList, it, this);
                combinedResult.m_modifiedMir |= r.m_modifiedMir;
                if (!r.m_succeeded)
                {
                    combinedResult.m_succeeded = false;
                    break;
                }
            }
            break;
        }
        case MirPassIterationPlace::Block:
        {
            for (auto *func : functionList)
            {
                auto &blocks = func->getBlocks();
                for (auto it = blocks.begin(); it != blocks.end(); ++it)
                {
                    MirPassResult r = pass->run(blocks, it, this);
                    combinedResult.m_modifiedMir |= r.m_modifiedMir;
                    if (!r.m_succeeded)
                    {
                        combinedResult.m_succeeded = false;
                        break;
                    }
                }
                if (!combinedResult.m_succeeded)
                {
                    break;
                }
            }
            break;
        }
        case MirPassIterationPlace::Instruction:
        {
            for (auto *func : functionList)
            {
                for (auto *block : func->getBlocks())
                {
                    auto &instructions = block->getInstructions();
                    for (auto it = instructions.begin(); it != instructions.end();)
                    {
                        auto nextIt = std::next(it);
                        MirPassResult r = pass->run(instructions, it, this);
                        combinedResult.m_modifiedMir |= r.m_modifiedMir;
                        if (!r.m_succeeded)
                        {
                            combinedResult.m_succeeded = false;
                            break;
                        }
                        it = nextIt;
                    }
                    if (!combinedResult.m_succeeded)
                    {
                        break;
                    }
                }
                if (!combinedResult.m_succeeded)
                {
                    break;
                }
            }
            break;
        }
    }

    // Invalidate cached analyses only once if the pass actually mutated MIR
    if (combinedResult.m_modifiedMir)
    {
        invalidateAnalysis();
    }

    pass->setResult(&combinedResult);
    m_savedResults[std::type_index(typeid(*pass))] = combinedResult;

    auto builder = m_diagCollector->trace("MirPassManager", "Pass result");
    builder.appendNote("Executed: {}", combinedResult.m_executed);
    builder.appendNote("Succeeded: {}", combinedResult.m_succeeded);
    builder.appendNote("Modified Mir: {}", combinedResult.m_modifiedMir);
    builder.flush();

    pass->printResult();

    return combinedResult;
}

void MirPassManager::invalidateAnalysis()
{
    for (auto &analysis : m_validAnalyses)
        analysis.second->reset();

    m_validAnalyses.clear();
}

void MirPassManager::runPipeline(MirBuilderContext *ctx)
{
    for (MirPass *pass : m_executionPipeline)
    {
        if (pass->getPassType() == MirPassType::Transform)
        {
            runPass(pass, ctx);
        }
    }
}

void MirPassManager::setTestMode() { m_testMode = true; }

MirPass *MirPassManager::runAnalysisById(std::type_index passId, MirBuilderContext *ctx)
{
    // Cache Check (handles downstream nested dependencies)
    if (m_validAnalyses.contains(passId))
    {
        return m_validAnalyses[passId];
    }

    auto it = m_passesBlueprint.find(passId);
    if (it == m_passesBlueprint.end())
    {
        throw std::runtime_error("Internal Compiler Error: Missing pass dependency in pass list");
    }

    MirPass *analysisPass = it->second.get();

    // Recursively resolve and cache all upstream prerequisites through the manager framework
    for (const auto &depId : analysisPass->getDependencies())
    {
        runAnalysisById(depId, ctx);
    }

    // Run the analysis and safely persist the result inside our map storage
    m_savedResults[passId] = runPass(analysisPass, ctx);
    analysisPass->setResult(&m_savedResults[passId]);

    // Validate cache entry tracking
    m_validAnalyses[passId] = analysisPass;

    return analysisPass;
}

void MirPassManager::generatePipeline()
{
    m_diagCollector->trace("MirPassManager", "Calculating pass dependency pipeline");

    m_executionPipeline.clear();
    std::unordered_set<std::type_index> resolved;
    std::unordered_set<std::type_index> seenInCurrentPath;

    for (const auto &[passId, passPtr] : m_passesBlueprint)
    {
        // Analysis passes are excluded from the static array loop; they invoke on-demand
        if (passPtr->getPassType() == MirPassType::Transform)
        {
            resolveDependencies(passId, resolved, seenInCurrentPath);
        }
    }

    auto builder = m_diagCollector->trace("MirPassManager", "Calculated pass dependency pipeline:");

    for (auto &pass : m_executionPipeline)
    {
        builder.appendNote(pass->getName());
    }
}

void MirPassManager::resolveDependencies(std::type_index passId,
                                         std::unordered_set<std::type_index> &resolved,
                                         std::unordered_set<std::type_index> &seenInCurrentPath)
{
    if (resolved.contains(passId))
        return;

    // Circular Dependency Guard
    if (seenInCurrentPath.contains(passId))
    {
        throw std::runtime_error("Fatal Compiler Error: Circular dependency detected in Pass Pipeline!");
    }

    auto it = m_passesBlueprint.find(passId);
    if (it == m_passesBlueprint.end())
    {
        throw std::runtime_error("Fatal Compiler Error: Missing pass dependency in Pass list");
    }

    if (!m_testMode)
    {
        seenInCurrentPath.insert(passId);
        for (const auto &depId : it->second->getDependencies())
        {
            resolveDependencies(depId, resolved, seenInCurrentPath);
        }
        seenInCurrentPath.erase(passId); // Backtrack
    }

    resolved.insert(passId);
    m_executionPipeline.push_back(it->second.get());
}