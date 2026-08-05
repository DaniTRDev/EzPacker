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
     * @brief Lazy-loads, executes, and caches an analysis pass utilizing the given context.
     */
    template <typename AnalysisPass>
        requires(std::is_base_of_v<IMirAnalysisPass, AnalysisPass>)
    AnalysisPass *getAnalysis(MirBuilderContext *ctx)
    {
        std::type_index passId = std::type_index(typeid(AnalysisPass));

        if (m_validAnalyses.contains(passId))
        {
            return static_cast<AnalysisPass *>(m_validAnalyses[passId]);
        }

        MirPass *executedPass = runAnalysisById(passId, ctx);
        return static_cast<AnalysisPass *>(executedPass);
    }

    /**
     * Invalidates the analysis stored.
     */
    void invalidateAnalysis();

    /**
     * Runs the generated pipeline (by generatePipeline) on the given context. Only TRANSFORM passes will be
     * executed, analysis passes will be run ONLY if they are required by any of the transform passes.
     * @param functionList
     */
    void runPipeline(MirBuilderContext *ctx);

    /**
     * When called, the pass manager enters in test mode, making it NOT RESOLVE dependencies.
     */
    void setTestMode();

    /**
     * Returns the diag collector linked to this pass manager.
     * @return
     */
    const std::shared_ptr<DiagnosticCollector> &getDiagCollector() const;

  private:
    /**
     * Internal implementation helper to resolve and execute an analysis pass by type_index.
     * @param passId
     * @param ctx
     */
    MirPass *runAnalysisById(std::type_index passId, MirBuilderContext *ctx);

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
     * Runs a TRANSFORMATION pass on the given place depending on its iteration type.
     * @param pass
     * @param ctx
     * @return
     */
    MirPassResult runPass(MirPass *pass, MirBuilderContext *ctx);

  private:
    bool m_testMode;
    std::pmr::unordered_map<std::type_index, MirPass *> m_validAnalyses;
    std::pmr::unordered_map<std::type_index, std::unique_ptr<MirPass>> m_passesBlueprint;
    std::pmr::vector<MirPass *> m_executionPipeline;
    // Storage to keep pass results alive safely in memory, preventing dangling pointer references
    std::pmr::unordered_map<std::type_index, MirPassResult> m_savedResults;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
};

#endif // EZPACKER_MIRPASSMANAGER_H