#ifndef EZPACKER_MIRPASSMANAGER_H
#define EZPACKER_MIRPASSMANAGER_H

#include "EzMirCommon.h"
#include "IMirAnalysisPass.h"
#include "IMirTransformPass.h"
#include "MirPass.h"
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
    template <typename PassType, typename... Args>
        requires(std::is_base_of_v<MirPass, PassType>)
    PassType *addPass(Args &&...args)
    {
        auto passId = std::type_index(typeid(PassType));
        m_passesBlueprint[passId] = std::make_unique<PassType>(std::forward<Args>(args)...);

        return static_cast<PassType *>(m_passesBlueprint[passId].get());
    }

    /**
     * Calculates the pipeline needed to run all the passes that have been pushed.
     */
    void generatePipeline();

    /**
     * @brief Lazy-loads, executes, and caches an analysis pass on demand.
     */
    template <typename AnalysisPass>
        requires(std::is_base_of_v<IMirAnalysisPass, AnalysisPass>)
    AnalysisPass *getAnalysis(std::pmr::list<class MirFunction *> &functionList)
    {
        std::type_index passId = std::type_index(typeid(AnalysisPass));

        // Cache Hit: Return the valid analysis instantly
        if (m_validAnalyses.contains(passId))
        {
            return static_cast<AnalysisPass *>(m_validAnalyses[passId]);
        }

        // Cache Miss: Query the non-templated backend engine to run and cache dynamically
        MirPass *executedPass = runAnalysisById(passId, functionList);
        return static_cast<AnalysisPass *>(executedPass);
    }

    /**
     * Runs the generated pipeline (by generatePipeline) on the given function list.
     * @param functionList
     */
    void runPipeline(std::pmr::list<class MirFunction *> &functionList);

    /**
     * Returns the diag collector linked to this pass manager.
     * @return
     */
    const std::shared_ptr<DiagnosticCollector> &getDiagCollector() const;

  private:
    /**
     * Internal implementation helper to resolve and execute an analysis pass by type_index.
     */
    MirPass *runAnalysisById(std::type_index passId, std::pmr::list<class MirFunction *> &functionList);

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
    MirPassResult runPass(MirPass *pass, std::pmr::list<class MirFunction *> &functionList);

  private:
    std::pmr::unordered_map<std::type_index, MirPass *> m_validAnalyses;
    std::pmr::unordered_map<std::type_index, std::unique_ptr<MirPass>> m_passesBlueprint;
    std::pmr::vector<MirPass *> m_executionPipeline;
    // Storage to keep pass results alive safely in memory, preventing dangling pointer references
    std::pmr::unordered_map<std::type_index, MirPassResult> m_savedResults;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
};

#endif // EZPACKER_MIRPASSMANAGER_H