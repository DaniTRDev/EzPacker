#ifndef EZMIR_MIR_PASS_MANAGER_H
#define EZMIR_MIR_PASS_MANAGER_H

#include "EzMirCommon.h"
#include "IMirAnalysisPass.h"
#include "IMirTransformPass.h"

class MirPassManager
{
  public:
    /**
     * Creates the pass manager and links it to the given arena.
     */
    MirPassManager(class DiagnosticCollector *diagCollector, std::pmr::memory_resource *globalArena);

    /**
     * Returns the diag collector linked to this pass manager.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Stashes a pass into the blueprint registry. It won't be ordered yet.
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
     * Lazy-loads, executes, and caches an analysis pass utilizing the given context.
     */
    template <typename AnalysisPass>
        requires(std::is_base_of_v<IMirAnalysisPass, AnalysisPass>)
    AnalysisPass *getAnalysis(class MirBuilderContext *ctx)
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
     * Runs a TRANSFORMATION pass on the given place depending on its iteration type.
     */
    MirPassResult runPass(MirPass *pass, class MirBuilderContext *ctx);

    /**
     * Invalidates the analysis stored.
     */
    void invalidateAnalysis();

    /**
     * Runs the generated pipeline (by generatePipeline) on the given context. Only TRANSFORM passes will be
     * executed, analysis passes will be run ONLY if they are required by any of the transform passes (through
     * getAnalysis).
     */
    void runPipeline(class MirBuilderContext *ctx);

    /**
     * When called, the pass manager enters in test mode, making it NOT RESOLVE dependencies.
     */
    void setTestMode();

  private:
    /**
     * Internal implementation helper to resolve and execute an analysis pass by type_index.
     */
    MirPass *runAnalysisById(std::type_index passId, class MirBuilderContext *ctx);

    /**
     * Tries to form a valid pass execution pipeline satisfying the dependencies of each pass.
     */
    void resolveDependencies(std::type_index passId,
                             std::unordered_set<std::type_index> &resolved,
                             std::unordered_set<std::type_index> &seenInCurrentPath);

  private:
    bool m_testMode;
    class DiagnosticCollector *m_diagCollector;
    std::pmr::unordered_map<std::type_index, MirPass *> m_validAnalyses;
    std::pmr::unordered_map<std::type_index, std::unique_ptr<MirPass>> m_passesBlueprint;
    // Storage to keep pass results alive safely in memory, preventing dangling pointer references
    std::pmr::unordered_map<std::type_index, MirPassResult> m_savedResults;
    std::pmr::vector<MirPass *> m_executionPipeline;
};

#endif // EZMIR_MIR_PASS_MANAGER_H