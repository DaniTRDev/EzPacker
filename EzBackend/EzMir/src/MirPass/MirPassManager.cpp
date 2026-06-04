#include "MirPass/MirPassManager.h"

MirPassManager::MirPassManager(std::pmr::memory_resource *globalArena,
                               std::shared_ptr<DiagnosticCollector> diagCollector) :
    m_validAnalyses(globalArena), m_passesBlueprint(globalArena), m_executionPipeline(globalArena),
    m_diagCollector(std::move(diagCollector))
{
}

void MirPassManager::runPipeline(std::pmr::list<MirFunction *> &functionList)
{
    // Ensure compilation has been executed at least once
    if (m_executionPipeline.empty() && !m_passesBlueprint.empty())
    {
        generatePipeline();
    }

    for (IMirPass *pass : m_executionPipeline)
    {
        // If it's a transform pass, clean up cached analyses to protect against stale data
        if (pass->getPassType() == MirPassType::Transform)
        {
            m_validAnalyses.clear();
        }

        runPass(pass, functionList);
    }
}

void MirPassManager::generatePipeline()
{
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager")
            << "Calculating pass dependency pipeline";

    m_executionPipeline.clear();
    std::unordered_set<std::type_index> resolved;
    std::unordered_set<std::type_index> seenInCurrentPath; // For cycle detection

    for (const auto &[passId, passPtr] : m_passesBlueprint)
    {
        resolveDependencies(passId, resolved, seenInCurrentPath);
    }

    auto builder = m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager");
    builder << "Calculated pass dependency pipeline";

    for (auto &pass : m_executionPipeline)
    {
        builder.appendNote(pass->getName(), nullptr);
    }
}

void MirPassManager::resolveDependencies(std::type_index passId,
                                         std::unordered_set<std::type_index> &resolved,
                                         std::unordered_set<std::type_index> &seenInCurrentPath)
{
    // 1. If already processed and added to the execution list, skip
    if (resolved.contains(passId))
        return;

    // 2. Circular Dependency Validation Guard
    if (seenInCurrentPath.contains(passId))
    {
        throw std::runtime_error("Fatal Compiler Error: Circular dependency detected in Pass Pipeline registry!");
    }

    auto it = m_passesBlueprint.find(passId);
    if (it == m_passesBlueprint.end())
    {
        throw std::runtime_error("Fatal Compiler Error: Missing pass dependency implementation in blueprint registry.");
    }

    // 3. Trace deeper into the graph
    seenInCurrentPath.insert(passId);
    for (const auto &depId : it->second->getDependencies())
    {
        resolveDependencies(depId, resolved, seenInCurrentPath);
    }
    seenInCurrentPath.erase(passId); // Backtrack

    // 4. Dependee is fully resolved. Safe to append Depender!
    resolved.insert(passId);
    m_executionPipeline.push_back(it->second.get());
}

const std::shared_ptr<DiagnosticCollector> &MirPassManager::getDiagCollector() const { return m_diagCollector; }

MirPassResult MirPassManager::runPass(IMirPass *pass, std::pmr::list<MirFunction *> &functionList)
{
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirPassManager")
            << std::pmr::string(std::format("Running pass {}", pass->getName()));

    MirPassResult result{};
    switch (pass->getIterationPlace())
    {
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
        case MirPassIterationPlace::Function:
        {
            for (auto func = functionList.begin(); func != functionList.end(); func++)
            {
                result = pass->run(functionList, func, this);
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
    builder.appendNote(std::pmr::string(std::format("Executed: {}", result.m_run)), nullptr);
    builder.appendNote(std::pmr::string(std::format("Succeeded: {}", result.m_succeeded)), nullptr);
    builder.appendNote(std::pmr::string(std::format("Modified Mir: {}", result.m_modifiedMir)), nullptr);

    return result;
}
