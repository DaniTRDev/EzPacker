#ifndef EZLINKER_DRIVER_H
#define EZLINKER_DRIVER_H

#include "EzLinkerCommon.h"
#include "SystemLinkerDetector.h"

namespace EzLinker
{

/**
 * Options controlling linker invocation, standard library injection, and target format.
 */
struct EZLINKER_API LinkerOptions
{
    std::vector<std::string> inputFiles;                 ///< Input object files (.o, .obj) or archives (.a, .lib).
    std::string outputFile;                              ///< Destination binary path.
    std::string customLinker;                            ///< Explicit linker executable override.
    std::string targetTriple;                            ///< Target architecture/platform triple.
    std::vector<std::string> libSearchPaths;             ///< Additional library search directories (-L).
    std::vector<std::string> libraries;                  ///< Additional libraries to link (-l).
    std::string entryPoint;                              ///< Custom entry point symbol name.
    std::string subsystem;                               ///< Subsystem (console, windows, etc.).
    std::filesystem::path exceptionRuntimePath;          ///< Optional override for EzExceptionRuntime path.
    std::vector<std::string> rawLinkerArgs;              ///< Raw arguments passed verbatim to the linker.

    bool isShared{ false };                              ///< Link as a shared library / DLL.
    bool isStatic{ false };                              ///< Link statically where applicable.
    bool verbose{ false };                               ///< Print verbose diagnostics and linker invocations.
    bool dryRun{ false };                                ///< Synthesize command line without executing the linker.
    bool noDefaultLibs{ false };                         ///< Do not automatically inject C standard libraries.
    bool noCompilerRt{ false };                          ///< Do not automatically inject compiler-rt / builtins.
    bool noExceptionRuntime{ false };                    ///< Do not automatically inject EzExceptionRuntime.
};

/**
 * Execution result of a linker run.
 */
struct EZLINKER_API LinkResult
{
    int exitCode{ 0 };
    std::string commandLine;
    std::string output;
    bool success{ false };
};

/**
 * High-level linker driver orchestrating system linker invocation, library discovery,
 * and command-line synthesis across Windows and Unix platforms.
 */
class EZLINKER_API EzLinkerDriver
{
public:
    EzLinkerDriver() = default;

    /**
     * Executes the linking pipeline with the given options.
     */
    LinkResult link(const LinkerOptions &opts) const;

    /**
     * Synthesizes the full command line arguments for the detected linker.
     */
    std::vector<std::string> buildLinkerArguments(const LinkerOptions &opts, const DetectedLinker &detectedLinker) const;

    /**
     * Returns the formatted single-line command string suitable for shell execution or display.
     */
    std::string buildCommandLineString(const std::filesystem::path &linkerPath, const std::vector<std::string> &args) const;

    /**
     * Discovers the built EzExceptionRuntime library file in the build tree or install prefix.
     */
    static std::filesystem::path findEzExceptionRuntimeLibrary(const std::string &targetTriple = "");

private:
    void appendMsvcArgs(const LinkerOptions &opts, std::vector<std::string> &args) const;
    void appendElfArgs(const LinkerOptions &opts, std::vector<std::string> &args) const;
    void appendDriverArgs(const LinkerOptions &opts, std::vector<std::string> &args) const;
};

} // namespace EzLinker

#endif // EZLINKER_DRIVER_H
