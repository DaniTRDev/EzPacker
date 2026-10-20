#ifndef EZPACKER_PIPELINE_TELEMETRY_H
#define EZPACKER_PIPELINE_TELEMETRY_H

#include "EzCompilerCommon.h"
#include "Diagnostics/DiagnosticListener.h"
#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/GenericSourceManager.h"

namespace EzCompiler
{

struct PassTelemetryRecord
{
    size_t index{ 0 };
    std::string passName;
    std::string stage;      // "Verification", "MiddleEnd", "Legalization", "TargetLowering"
    std::string functionName;
    std::string passType;   // "Transform" or "Analysis"
    double durationMs{ 0.0 };
    bool modifiedMir{ false };
    size_t instructionCount{ 0 };
    size_t blockCount{ 0 };
    std::string mirDump;
};

struct DiagnosticTelemetryRecord
{
    std::string severity;   // "error", "warning", "trace", "debug"
    std::string component;
    std::string message;
    std::string file;
    size_t line{ 0 };
    size_t col{ 0 };
};

struct CfgBlockTelemetry
{
    std::string name;
    uint64_t id{ 0 };
    size_t instructionCount{ 0 };
    std::vector<std::string> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
};

struct LivenessBlockTelemetry
{
    std::string blockName;
    uint64_t blockId{ 0 };
    std::vector<std::string> def;
    std::vector<std::string> use;
    std::vector<std::string> liveIn;
    std::vector<std::string> liveOut;
};

struct FrameLayoutTelemetry
{
    bool isComputed{ false };
    size_t calleeSaveSize{ 0 };
    size_t totalSize{ 0 };
    bool hasCalls{ false };
    bool hasDynamicAllocs{ false };
    std::vector<std::string> usedCalleeSavedRegs;
};

struct FunctionTelemetry
{
    std::string name;
    std::vector<CfgBlockTelemetry> blocks;
    std::vector<LivenessBlockTelemetry> liveness;
    FrameLayoutTelemetry frameLayout;
};

/**
 * Diagnostic observer and telemetry collector recording pass-by-pass snapshots,
 * metrics, CFG topologies, liveness intervals, and frame layout for compiler developer tooling.
 */
class PipelineTelemetry : public DiagnosticListener
{
  public:
    PipelineTelemetry();
    ~PipelineTelemetry() override = default;

    void setTarget(std::string_view target) { m_target = target; }
    void setOptLevel(std::string_view opt) { m_optLevel = opt; }
    void setInputFile(std::string_view file) { m_inputFile = file; }
    void setInitialMir(std::string mir) { m_initialMir = std::move(mir); }
    void setAssembly(std::string asmText) { m_assembly = std::move(asmText); }

    void recordPass(PassTelemetryRecord record);
    void updateFunctionAnalysis(const std::string &fnName, FunctionTelemetry telemetry);

    void onDiag(const DiagnosticMessage &msg) override;
    void bindSourceManager(GenericSourceManager *srcMgr) { m_srcMgr = srcMgr; }

    [[nodiscard]] std::string toJson() const;
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
    GenericSourceManager *m_srcMgr{ nullptr };
};

} // namespace EzCompiler

#endif // EZPACKER_PIPELINE_TELEMETRY_H

