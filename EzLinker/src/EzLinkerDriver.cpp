#include "EzLinkerDriver.h"
#include <cstdlib>
#include <sstream>

#if defined(_WIN32)
#define EZ_POPEN _popen
#define EZ_PCLOSE _pclose
#else
#define EZ_POPEN popen
#define EZ_PCLOSE pclose
#endif

namespace EzLinker
{

std::filesystem::path EzLinkerDriver::findEzExceptionRuntimeLibrary(const std::string &targetTriple)
{
    // Search current directory, build/lib, and relative paths
    std::vector<std::filesystem::path> searchRoots = {
        std::filesystem::current_path(),
        std::filesystem::current_path() / "lib",
        std::filesystem::current_path() / "bin",
        std::filesystem::current_path() / "cmake-build-debug" / "lib",
        std::filesystem::current_path() / "cmake-build-release" / "lib"
    };

    std::vector<std::string> libNames = {
        "EzExceptionRuntime.lib",
        "libEzExceptionRuntime.a",
        "EzExceptionRuntime.a",
        "libEzExceptionRuntime.so",
        "EzExceptionRuntime.dll"
    };

    for (const auto &root : searchRoots)
    {
        for (const auto &name : libNames)
        {
            std::filesystem::path candidate = root / name;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec))
            {
                return candidate;
            }
        }
    }

    return {};
}

std::string EzLinkerDriver::buildCommandLineString(const std::filesystem::path &linkerPath,
                                                   const std::vector<std::string> &args) const
{
    std::ostringstream ss;
    ss << "\"" << linkerPath.string() << "\"";
    for (const auto &arg : args)
    {
        if (arg.find(' ') != std::string::npos && !arg.starts_with("\""))
        {
            ss << " \"" << arg << "\"";
        }
        else
        {
            ss << " " << arg;
        }
    }
    return ss.str();
}

void EzLinkerDriver::appendMsvcArgs(const LinkerOptions &opts, std::vector<std::string> &args) const
{
    args.push_back("/nologo");

    // Output binary path
    std::string outPath = opts.outputFile.empty() ? "a.exe" : opts.outputFile;
    args.push_back("/out:" + outPath);

    // Input object files
    for (const auto &in : opts.inputFiles)
    {
        args.push_back(in);
    }

    // User-provided library search paths
    for (const auto &p : opts.libSearchPaths)
    {
        args.push_back("/libpath:" + p);
    }

    // User-provided libraries
    for (const auto &lib : opts.libraries)
    {
        std::string libName = lib;
        if (!libName.ends_with(".lib"))
        {
            libName += ".lib";
        }
        args.push_back(libName);
    }

    // Default MSVC & Windows SDK library paths if not disabled
    if (!opts.noDefaultLibs)
    {
        auto msvcPaths = SystemLinkerDetector::getKnownMsvcLibPaths();
        for (const auto &mp : msvcPaths)
        {
            args.push_back("/libpath:" + mp.string());
        }

        // C Runtime libraries
        if (opts.isStatic)
        {
            args.push_back("libcmt.lib");
            args.push_back("libvcruntime.lib");
            args.push_back("libucrt.lib");
        }
        else
        {
            args.push_back("msvcrt.lib");
            args.push_back("vcruntime.lib");
            args.push_back("ucrt.lib");
        }

        // Win32 base libraries
        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
    }

    // Compiler-rt / legalizer helpers
    if (!opts.noCompilerRt)
    {
        auto rtPaths = SystemLinkerDetector::getKnownCompilerRtPaths();
        for (const auto &rt : rtPaths)
        {
            args.push_back("/libpath:" + rt.string());
        }
    }

    // Exception runtime helper
    if (!opts.noExceptionRuntime)
    {
        std::filesystem::path exRtPath = opts.exceptionRuntimePath;
        if (exRtPath.empty())
        {
            exRtPath = findEzExceptionRuntimeLibrary(opts.targetTriple);
        }

        if (!exRtPath.empty())
        {
            if (exRtPath.has_parent_path())
            {
                args.push_back("/libpath:" + exRtPath.parent_path().string());
            }
            args.push_back(exRtPath.filename().string());
        }
        else
        {
            args.push_back("EzExceptionRuntime.lib");
        }
    }

    // Subsystem and entry point
    if (!opts.entryPoint.empty())
    {
        args.push_back("/entry:" + opts.entryPoint);
    }

    if (opts.isShared)
    {
        args.push_back("/dll");
    }
    else
    {
        std::string sub = opts.subsystem.empty() ? "console" : opts.subsystem;
        args.push_back("/subsystem:" + sub);
    }

    // Verbatim raw linker arguments
    for (const auto &raw : opts.rawLinkerArgs)
    {
        args.push_back(raw);
    }
}

void EzLinkerDriver::appendElfArgs(const LinkerOptions &opts, std::vector<std::string> &args) const
{
    // Output binary path
    std::string outPath = opts.outputFile.empty() ? "a.out" : opts.outputFile;
    args.push_back("-o");
    args.push_back(outPath);

    // Input object files
    for (const auto &in : opts.inputFiles)
    {
        args.push_back(in);
    }

    // User-provided library search paths
    for (const auto &p : opts.libSearchPaths)
    {
        args.push_back("-L" + p);
    }

    // User-provided libraries
    for (const auto &lib : opts.libraries)
    {
        args.push_back("-l" + lib);
    }

    // Default C runtime libraries
    if (!opts.noDefaultLibs)
    {
        args.push_back("-lc");
        args.push_back("-lm");
        args.push_back("-lpthread");
        args.push_back("-ldl");
    }

    // Compiler-rt / legalizer helpers
    if (!opts.noCompilerRt)
    {
        args.push_back("-lgcc");
    }

    // Exception runtime helper
    if (!opts.noExceptionRuntime)
    {
        std::filesystem::path exRtPath = opts.exceptionRuntimePath;
        if (exRtPath.empty())
        {
            exRtPath = findEzExceptionRuntimeLibrary(opts.targetTriple);
        }

        if (!exRtPath.empty())
        {
            if (exRtPath.has_parent_path())
            {
                args.push_back("-L" + exRtPath.parent_path().string());
            }
            std::string stem = exRtPath.stem().string();
            if (stem.starts_with("lib"))
            {
                stem = stem.substr(3);
            }
            args.push_back("-l" + stem);
        }
        else
        {
            args.push_back("-lEzExceptionRuntime");
        }
    }

    if (opts.isShared)
    {
        args.push_back("-shared");
    }
    if (opts.isStatic)
    {
        args.push_back("-static");
    }
    if (!opts.entryPoint.empty())
    {
        args.push_back("-e");
        args.push_back(opts.entryPoint);
    }

    // Verbatim raw linker arguments
    for (const auto &raw : opts.rawLinkerArgs)
    {
        args.push_back(raw);
    }
}

void EzLinkerDriver::appendDriverArgs(const LinkerOptions &opts, std::vector<std::string> &args) const
{
    std::string outPath = opts.outputFile.empty() ? "a.out" : opts.outputFile;
    args.push_back("-o");
    args.push_back(outPath);

    for (const auto &in : opts.inputFiles)
    {
        args.push_back(in);
    }

    for (const auto &p : opts.libSearchPaths)
    {
        args.push_back("-L" + p);
    }

    for (const auto &lib : opts.libraries)
    {
        args.push_back("-l" + lib);
    }

    if (opts.isShared)
    {
        args.push_back("-shared");
    }
    if (opts.isStatic)
    {
        args.push_back("-static");
    }

    if (!opts.noExceptionRuntime)
    {
        std::filesystem::path exRtPath = opts.exceptionRuntimePath;
        if (exRtPath.empty())
        {
            exRtPath = findEzExceptionRuntimeLibrary(opts.targetTriple);
        }

        if (!exRtPath.empty())
        {
            if (exRtPath.has_parent_path())
            {
                args.push_back("-L" + exRtPath.parent_path().string());
            }
            std::string stem = exRtPath.stem().string();
            if (stem.starts_with("lib"))
            {
                stem = stem.substr(3);
            }
            args.push_back("-l" + stem);
        }
        else
        {
            args.push_back("-lEzExceptionRuntime");
        }
    }

    for (const auto &raw : opts.rawLinkerArgs)
    {
        args.push_back(raw);
    }
}

std::vector<std::string> EzLinkerDriver::buildLinkerArguments(const LinkerOptions &opts,
                                                             const DetectedLinker &detectedLinker) const
{
    std::vector<std::string> args;
    if (detectedLinker.isMsvcCompatible())
    {
        appendMsvcArgs(opts, args);
    }
    else if (detectedLinker.isElfLinker())
    {
        appendElfArgs(opts, args);
    }
    else if (detectedLinker.isCompilerDriver())
    {
        appendDriverArgs(opts, args);
    }
    else
    {
        // Fallback generic args
        appendElfArgs(opts, args);
    }
    return args;
}

LinkResult EzLinkerDriver::link(const LinkerOptions &opts) const
{
    LinkResult res;

    DetectedLinker linker = SystemLinkerDetector::detect(opts.customLinker, opts.targetTriple);
    if (!linker.isValid())
    {
        res.exitCode = 1;
        res.output = "error: no suitable system linker found (lld, link.exe, ld, etc.)";
        res.success = false;
        return res;
    }

    auto args = buildLinkerArguments(opts, linker);
    res.commandLine = buildCommandLineString(linker.path, args);

    if (opts.verbose)
    {
        std::cout << "[EzLinker] Linker: " << linker.path.string() << " (" << linker.getKindName() << ")\n";
        std::cout << "[EzLinker] Command: " << res.commandLine << "\n";
    }

    if (opts.dryRun)
    {
        res.exitCode = 0;
        res.success = true;
        res.output = res.commandLine;
        return res;
    }

    std::string runCmd = res.commandLine + " 2>&1";
    FILE *pipe = EZ_POPEN(runCmd.c_str(), "r");
    if (!pipe)
    {
        res.exitCode = 1;
        res.output = "error: failed to spawn linker process: " + linker.path.string();
        res.success = false;
        return res;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        res.output += buffer;
    }

    res.exitCode = EZ_PCLOSE(pipe);
    res.success = (res.exitCode == 0);

    return res;
}

} // namespace EzLinker
