#ifndef EZPACKER_MIRPASSMANAGER_H
#define EZPACKER_MIRPASSMANAGER_H

#include "EzMirCommon.h"
#include "IMirAnalysisPass.h"
#include "IMirTransformPass.h"
#include "IMirPass.h"
#include "Function/MirFunction.h"
#include "Printer/MirPrinter.h"

class MirPassManager
{
  public:
    /**
     * Creates the pass manager and links it to the given arena.
     * @param globalArena
     * @param diagCollector
     */
    MirPassManager(std::pmr::memory_resource *globalArena, std::shared_ptr<DiagnosticCollector> diagCollector);

    /**
     * @brief Stashes a pass into the blueprint registry. It won't be ordered yet.
     */
    template <typename T, typename... Args>
        requires(std::is_base_of_v<IMirPass, T>)
    void addPass(Args &&...args)
    {
        auto passId = std::type_index(typeid(T));
        m_passesBlueprint[passId] = std::make_unique<T>(std::forward<Args>(args)...);
    }

    /**
     * Calculates the pipeline needed to run all the passes that have been pushed.
     */
    void generatePipeline();

    template <typename AnalysisPass>
        requires(std::is_base_of_v<IMirAnalysisPass, AnalysisPass>)
    AnalysisPass *getAnalysis(std::pmr::list<class MirFunction *> &functionList)
    {
        std::type_index typeId = std::type_index(typeid(AnalysisPass));

        // Check if the analysis pass has already run and its cached result is valid
        auto it = m_validAnalyses.find(typeId);
        if (it != m_validAnalyses.end())
        {
            return static_cast<AnalysisPass *>(it->second);
        }

        // Not found in cache. Lookup the pass instance inside the blueprint registry
        auto blueprintIt = m_passesBlueprint.find(typeId);
        AnalysisPass *passInstance = nullptr;

        if (blueprintIt != m_passesBlueprint.end())
        {
            // Use the pre-registered pass instance from the blueprint graph
            passInstance = static_cast<AnalysisPass *>(blueprintIt->second.get());
        }
        else
        {
            m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirPassManager")
                    << "Tried to run a pass that has not been previously added";
            throw std::runtime_error("");
        }

        MirPassResult result = runPass(passInstance, functionList);
        if (result.m_run && result.m_succeeded)
        {
            // Cache the pointer so future passes can access it instantly without re-running
            m_validAnalyses[typeId] = passInstance;
        }

        return passInstance;
    }

    /**
     * Runs the generated pipeline (by generatePipeline) on the given function list.
     * @param codeModule
     */
    void runPipeline(std::pmr::list<class MirFunction *> &functionList);

    /**
     * Returns the diag collector linked to this pass manager.
     * @return
     */
    const std::shared_ptr<DiagnosticCollector> &getDiagCollector() const;

  private:
    /**
     * Tries to form a valid pass execution pipeline satisfying the dependencies of each pass.
     * @param passId
     * @param resolved
     * @param seenInCurrentPath
     */
    void resolveDependencies(std::type_index passId,
                             std::unordered_set<std::type_index> &resolved,
                             std::unordered_set<std::type_index> &seenInCurrentPath);

    /**
     * Runs a pass on the given place depending on its iteration type.
     * @param pass
     * @return
     */
    MirPassResult runPass(IMirPass *pass, std::pmr::list<class MirFunction *> &functionList);

  private:
    std::pmr::unordered_map<std::type_index, IMirPass *>
            m_validAnalyses; // Analysis passes that have been run and have returned data.
    std::pmr::unordered_map<std::type_index, std::unique_ptr<IMirPass>>
            m_passesBlueprint; // Links an index to its pass.
    std::pmr::vector<IMirPass *>
            m_executionPipeline; // An ordered list of passes that guarantees that every dep is resolved.
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
};

#endif // EZPACKER_MIRPASSMANAGER_H
