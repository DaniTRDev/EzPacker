#include "MirPasses/MirPassManager.h"

MirPassManager::MirPassManager(std::pmr::memory_resource *globalArena,
                               std::shared_ptr<DiagnosticCollector> diagCollector) :
    m_validAnalyses(globalArena), m_passesBlueprint(globalArena), m_executionPipeline(globalArena),
    m_savedResults(globalArena), m_diagCollector(std::move(diagCollector))
{
}

void MirPassManager::runPipeline(std::pmr::list<MirFunction *> &functionList)
{
    // Ensure compilation pipeline has been calculated at least once
    if (m_executionPipeline.empty() && !m_passesBlueprint.empty())
    {
        generatePipeline();
    }

    for (MirPass *pass : m_executionPipeline)
    {
        // Pipeline loop only processes Transform / Optimization passes sequentially
        if (pass->getPassType() == MirPassType::Transform)
        {
            // Clean up cached analyses to protect against stale data mutations
            m_validAnalyses.clear();
            m_savedResults.clear();

            auto passId = std::type_index(typeid(*pass));
            m_savedResults[passId] = runPass(pass, functionList);
            pass->setResult(&m_savedResults[passId]);
        }
    }
}

MirPass *MirPassManager::runAnalysisById(std::type_index passId, std::pmr::list<class MirFunction *> &functionList)
{
    // Cache Check (handles downstream nested dependencies)
    if (m_validAnalyses.contains(passId))
    {
        return m_validAnalyses[passId];
    }

    auto it = m_passesBlueprint.find(passId);
    if (it == m_passesBlueprint.end())
    {
        throw std::runtime_error(
                "Internal Compiler Error: Missing pass dependency implementation in blueprint registry.");
    }

    MirPass *analysisPass = it->second.get();

    // Recursively resolve and cache all upstream prerequisites through the manager framework
    for (const auto &depId : analysisPass->getDependencies())
    {
        runAnalysisById(depId, functionList);
    }

    // Run the analysis and safely persist the result inside our map storage
    m_savedResults[passId] = runPass(analysisPass, functionList);
    analysisPass->setResult(&m_savedResults[passId]);

    // Validate cache entry tracking
    m_validAnalyses[passId] = analysisPass;

    return analysisPass;
}

void MirPassManager::generatePipeline()
{
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager")
            << "Calculating pass dependency pipeline";

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

    auto builder = m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager");
    builder << "Calculated pass dependency pipeline:";

    for (auto &pass : m_executionPipeline)
    {
        builder.appendNote(pass->getName(), nullptr);
    }
}

void MirPassManager::resolveDependencies(std::type_index passId,
                                         std::unordered_set<std::type_index> &resolved,
                                         std::unordered_set<std::type_index> &seenInCurrentPath)
{
    if (resolved.contains(passId))
        return;

    // Circular Dependency Validation Guard
    if (seenInCurrentPath.contains(passId))
    {
        throw std::runtime_error("Fatal Compiler Error: Circular dependency detected in Pass Pipeline registry!");
    }

    auto it = m_passesBlueprint.find(passId);
    if (it == m_passesBlueprint.end())
    {
        throw std::runtime_error("Fatal Compiler Error: Missing pass dependency implementation in blueprint registry.");
    }

    seenInCurrentPath.insert(passId);
    for (const auto &depId : it->second->getDependencies())
    {
        resolveDependencies(depId, resolved, seenInCurrentPath);
    }
    seenInCurrentPath.erase(passId); // Backtrack

    resolved.insert(passId);
    m_executionPipeline.push_back(it->second.get());
}

const std::shared_ptr<DiagnosticCollector> &MirPassManager::getDiagCollector() const { return m_diagCollector; }

MirPassResult MirPassManager::runPass(MirPass *pass, std::pmr::list<MirFunction *> &functionList)
{
    auto log = m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager");
    log << std::pmr::string(std::format("Running pass {}", pass->getName()));
    log.flush();

    MirPassResult result{};
    switch (pass->getIterationPlace())
    {
        case MirPassIterationPlace::Function:
        {
            for (auto func = functionList.begin(); func != functionList.end(); func++)
            {
                result = pass->run(functionList, func, this);
            }
            break;
        }
        case MirPassIterationPlace::Block:
        {
            for (auto func : functionList)
            {
                auto blockList = func->getBlocks();
                for (auto block = blockList.begin(); block != blockList.end(); block++)
                {
                    result = pass->run(blockList, block, this);
                }
            }
            break;
        }
        case MirPassIterationPlace::Instruction:
        {
            for (auto func : functionList)
            {
                auto blockList = func->getBlocks();
                for (auto block = blockList.begin(); block != blockList.end(); block++)
                {
                    auto instrList = (*block)->getInstructions();
                    for (auto instr = instrList.begin(); instr != instrList.end(); instr++)
                    {
                        result = pass->run(blockList, block, this);
                    }
                }
            }
            break;
        }
    }

    auto builder = m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager");
    builder << "Pass result";
    builder.appendNote(std::pmr::string(std::format("Executed: {}", result.m_executed)), nullptr);
    builder.appendNote(std::pmr::string(std::format("Succeeded: {}", result.m_succeeded)), nullptr);
    builder.appendNote(std::pmr::string(std::format("Modified Mir: {}", result.m_modifiedMir)), nullptr);
    builder.flush();

    pass->printResult();
    return result;
}