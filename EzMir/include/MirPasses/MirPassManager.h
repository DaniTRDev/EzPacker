#ifndef EZMIR_MIR_PASS_MANAGER_H
#define EZMIR_MIR_PASS_MANAGER_H

#include "EzMirCommon.h"
#include "IMirAnalysisPass.h"
#include "IMirTransformPass.h"

/**
 * Orchestrator managing pass dependency resolution, dynamic topological pipeline construction,
 * on-demand analysis caching and invalidation, and iterative execution over MIR data structures.
 *
 * Error contract (pass-manager layer): a violation of an internal invariant (a requested pass was
 * never registered, or the dependency graph contains a cycle) throws std::runtime_error. Ordinary
 * pass execution failures are not exceptions: they are reported through the returned
 * MirPassResult::m_succeeded flag and the DiagnosticCollector.
 */
class MirPassManager
{
  public:
    /**
     * Constructs a pass manager bound to a DiagnosticCollector and arena memory resource.
     */
    MirPassManager(class DiagnosticCollector *diagCollector, std::pmr::memory_resource *globalArena);

    /**
     * Returns the diagnostic collector attached to this pass manager.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Registers a pass blueprint with constructor arguments into the manager.
     */
    template <typename PassType, typename... Args>
        requires(std::is_base_of_v<MirPass, PassType>)
    PassType *addPass(Args &&...args)
    {
        auto passId = std::type_index(typeid(PassType));
        auto &slot = m_passesBlueprint[passId];
        slot = std::make_unique<PassType>(std::forward<Args>(args)...);

        return static_cast<PassType *>(slot.get());
    }

    /**
     * Computes the topologically sorted execution pipeline resolving all declared pass dependencies.
     */
    void generatePipeline();

    /**
     * Retrieves an analysis pass result on-demand, executing the analysis pass if not already cached.
     */
    template <typename AnalysisPass>
        requires(std::is_base_of_v<IMirAnalysisPass, AnalysisPass>)
    AnalysisPass *getAnalysis(class MirBuilderContext *ctx)
    {
        std::type_index passId = std::type_index(typeid(AnalysisPass));

        if (auto it = m_validAnalyses.find(passId); it != m_validAnalyses.end())
        {
            return static_cast<AnalysisPass *>(it->second);
        }

        MirPass *executedPass = runAnalysisById(passId, ctx);
        return static_cast<AnalysisPass *>(executedPass);
    }

    /**
     * Executes a transformation pass across its configured granularity place (Function, Block, Instruction, GlobalVar).
     */
    MirPassResult runPass(MirPass *pass, class MirBuilderContext *ctx);

    /**
     * Invalidates all cached analysis results following a mutating transformation pass.
     */
    void invalidateAnalysis();

    /**
     * Executes the generated pipeline of transformation passes over all functions/globals in the context.
     */
    void runPipeline(class MirBuilderContext *ctx);

    /**
     * Enables test mode, allowing passes to run without enforcing prerequisite dependency resolution.
     */
    void setTestMode();

  private:
    /**
     * Internal implementation helper to resolve and execute an analysis pass by type_index.
     */
    MirPass *runAnalysisById(std::type_index passId, class MirBuilderContext *ctx);

    /**
     * Recursively traverses and resolves dependencies for a pass, detecting cyclic dependency chains.
     */
    void resolveDependencies(std::type_index passId,
                             std::unordered_set<std::type_index> &resolved,
                             std::unordered_set<std::type_index> &seenInCurrentPath);

  private:
    bool m_testMode;                            // When true, dependency enforcement is relaxed for tests.
    class DiagnosticCollector *m_diagCollector; // Collector used to report pass errors.
    std::pmr::unordered_map<std::type_index, MirPass *>
            m_validAnalyses; // Cache of analyses valid for the current MIR state.
    std::pmr::unordered_map<std::type_index, std::unique_ptr<MirPass>>
            m_passesBlueprint; // Registered pass instances keyed by type.
    // Storage to keep pass results alive safely in memory, preventing dangling pointer references
    std::pmr::unordered_map<std::type_index, MirPassResult> m_savedResults; // Stable storage for per-pass results.
    std::pmr::vector<MirPass *> m_executionPipeline; // Topologically ordered transform passes to run.
};

#endif // EZMIR_MIR_PASS_MANAGER_H