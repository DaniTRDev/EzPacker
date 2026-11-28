#ifndef EZPACKER_PIPELINE_TELEMETRY_H
#define EZPACKER_PIPELINE_TELEMETRY_H

#include "EzCompilerCommon.h"
#include "Diagnostics/DiagnosticListener.h"
#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/GenericSourceManager.h"

namespace EzCompiler
{

/**
 * Telemetry snapshot for a single compiler pass execution.
 */
struct PassTelemetryRecord
{
    size_t index{ 0 };            ///< 0-based sequential pass execution index.
    std::string passName;         ///< Registered pass name.
    std::string stage;            ///< Pipeline stage ("Verification", "MiddleEnd", "Legalization", "TargetLowering").
    std::string functionName;     ///< Target function name (or module-level label).
    std::string passType;         ///< Pass classification ("Transform" or "Analysis").
    double durationMs{ 0.0 };     ///< Pass wall-clock duration in milliseconds.
    bool modifiedMir{ false };    ///< True if the pass modified the MIR.
    size_t instructionCount{ 0 }; ///< Instruction count after the pass.
    size_t blockCount{ 0 };       ///< Basic block count after the pass.
    std::string mirDump;          ///< Textual MIR dump captured after the pass.
};

/**
 * Telemetry record for a single compiler diagnostic message.
 */
struct DiagnosticTelemetryRecord
{
    std::string severity;  ///< Diagnostic severity ("error", "warning", "trace", "debug").
    std::string component; ///< Originating compiler subsystem.
    std::string message;   ///< Formatted diagnostic text.
    std::string file;      ///< Source file path.
    size_t line{ 0 };      ///< 1-based source line.
    size_t col{ 0 };       ///< 1-based source column.
};

/**
 * Control-flow graph telemetry for a single basic block.
 */
struct CfgBlockTelemetry
{
    std::string name;                  ///< Basic block label.
    uint64_t id{ 0 };                  ///< Numeric block ID.
    size_t instructionCount{ 0 };      ///< Number of instructions in the block.
    std::vector<std::string> instructions; ///< Formatted instruction strings.
    std::vector<std::string> predecessors; ///< Predecessor block labels.
    std::vector<std::string> successors;   ///< Successor block labels.
};

/**
 * Liveness analysis sets (def/use/live-in/live-out) for a single basic block.
 */
struct LivenessBlockTelemetry
{
    std::string blockName;            ///< Basic block label.
    uint64_t blockId{ 0 };            ///< Numeric block ID.
    std::vector<std::string> def;     ///< Registers defined in the block.
    std::vector<std::string> use;     ///< Registers used before definition in the block.
    std::vector<std::string> liveIn;  ///< Registers live at block entry.
    std::vector<std::string> liveOut; ///< Registers live at block exit.
};

/**
 * Stack frame layout metrics for a compiled function.
 */
struct FrameLayoutTelemetry
{
    bool isComputed{ false };                     ///< True if frame layout has been finalized.
    size_t calleeSaveSize{ 0 };                   ///< Bytes reserved for callee-saved registers.
    size_t totalSize{ 0 };                        ///< Total aligned stack frame size in bytes.
    bool hasCalls{ false };                       ///< True if the function performs calls.
    bool hasDynamicAllocs{ false };               ///< True if the function has dynamic stack allocations.
    std::vector<std::string> usedCalleeSavedRegs; ///< Names of callee-saved registers preserved by the prologue.
};

/**
 * Aggregated per-function CFG, liveness, and stack frame telemetry.
 */
struct FunctionTelemetry
{
    std::string name;                              ///< Function symbol name.
    std::vector<CfgBlockTelemetry> blocks;         ///< CFG block snapshots.
    std::vector<LivenessBlockTelemetry> liveness;  ///< Per-block liveness sets.
    FrameLayoutTelemetry frameLayout;              ///< Finalized stack frame layout.
};

/**
 * Summary telemetry for debug information emission.
 */
struct DebugInfoTelemetry
{
    bool enabled{ false };                    ///< True if debug info emission was enabled (-g1 / -g2).
    std::string format;                       ///< Resolved debug format ("DWARF5", "CodeView", or "None").
    std::string level;                        ///< Debug granularity ("None", "LineTablesOnly", "Full").
    size_t fileCount{ 0 };                    ///< Number of registered source files.
    size_t typeCount{ 0 };                    ///< Number of emitted debug type entries.
    size_t functionCount{ 0 };                ///< Number of emitted debug function entries.
    size_t globalCount{ 0 };                  ///< Number of emitted debug global variable entries.
    size_t lineEntryCount{ 0 };               ///< Total number of emitted line table entries.
    std::vector<std::string> emittedSections; ///< Names of generated debug object sections.
};

/**
 * Diagnostic observer and telemetry collector recording pass-by-pass snapshots,
 * metrics, CFG topologies, liveness intervals, and frame layout for compiler developer tooling.
 */
class PipelineTelemetry : public DiagnosticListener
{
  public:
    /**
     * Constructs an empty telemetry collector.
     */
    PipelineTelemetry();
    ~PipelineTelemetry() override = default;

    /** Sets the target triple string for the compilation run. */
    void setTarget(std::string_view target) { m_target = target; }
    /** Sets the optimization level string (e.g., "O0", "O2"). */
    void setOptLevel(std::string_view opt) { m_optLevel = opt; }
    /** Sets the primary input source file path. */
    void setInputFile(std::string_view file) { m_inputFile = file; }
    /** Records the initial MIR dump prior to optimization passes. */
    void setInitialMir(std::string mir) { m_initialMir = std::move(mir); }
    /** Records the final emitted assembly text, if applicable. */
    void setAssembly(std::string asmText) { m_assembly = std::move(asmText); }
    /** Records debug information emission summary metrics. */
    void setDebugInfo(DebugInfoTelemetry debugInfo) { m_debugInfo = std::move(debugInfo); }

    /** Appends a single pass execution record. */
    void recordPass(PassTelemetryRecord record);
    /** Stores or updates per-function CFG, liveness, and frame layout telemetry. */
    void updateFunctionAnalysis(const std::string &fnName, FunctionTelemetry telemetry);

    /** DiagnosticListener callback capturing emitted compiler diagnostics. */
    void onDiag(const DiagnosticMessage &msg) override;
    /** Binds a source manager used to resolve source file paths for diagnostics. */
    void bindSourceManager(GenericSourceManager *srcMgr) { m_srcMgr = srcMgr; }

    /** Serializes the collected pipeline telemetry to a JSON string. */
    [[nodiscard]] std::string toJson() const;
    /** Writes the serialized JSON telemetry to disk at the given path. */
    bool writeToFile(const std::filesystem::path &path) const;

  private:
    std::string m_target;
    std::string m_optLevel;
    std::string m_inputFile;
    std::string m_initialMir;
    std::string m_assembly;
    std::vector<PassTelemetryRecord> m_passes;
    std::vector<DiagnosticTelemetryRecord> m_diagnostics;
    std::unordered_map<std::string, FunctionTelemetry> m_functions;
    DebugInfoTelemetry m_debugInfo;
    GenericSourceManager *m_srcMgr{ nullptr };
};

} // namespace EzCompiler

#endif // EZPACKER_PIPELINE_TELEMETRY_H

