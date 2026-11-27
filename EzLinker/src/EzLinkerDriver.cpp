#include "EzLinkerDriver.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define EZ_POPEN _popen
#define EZ_PCLOSE _pclose
#else
#include <sys/wait.h>
#include <unistd.h>
#define EZ_POPEN popen
#define EZ_PCLOSE pclose
#endif

namespace EzLinker
{

namespace
{

bool isWindowsTargetTriple(const std::string &targetTriple)
{
    if (!targetTriple.empty())
    {
        std::string lower = targetTriple;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lower.find("windows") != std::string::npos ||
               lower.find("win32") != std::string::npos ||
               lower.find("msvc") != std::string::npos ||
               lower.find("coff") != std::string::npos;
    }
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

std::filesystem::path getCurrentExecutableDir()
{
#if defined(_WIN32)
    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        return std::filesystem::path(exePath).parent_path();
    }
#elif defined(__linux__)
    char exePath[4096];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0)
    {
        exePath[len] = '\0';
        return std::filesystem::path(exePath).parent_path();
    }
#endif
    return {};
}

std::vector<std::filesystem::path> buildLibrarySearchRoots()
{
    std::vector<std::filesystem::path> roots;

    std::filesystem::path exeDir = getCurrentExecutableDir();
    if (!exeDir.empty())
    {
        roots.push_back(exeDir);
        roots.push_back(exeDir / "lib");
        roots.push_back(exeDir / ".." / "lib");
        roots.push_back(exeDir / ".." / ".." / "lib");
    }

    std::error_code ec;
    std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!cwd.empty())
    {
        roots.push_back(cwd);
        roots.push_back(cwd / "lib");
        roots.push_back(cwd / "bin");
        roots.push_back(cwd / "build" / "lib");
        roots.push_back(cwd / "build" / "bin");
        roots.push_back(cwd / "build" / "linux" / "lib");
        roots.push_back(cwd / "cmake-build-debug" / "lib");
        roots.push_back(cwd / "cmake-build-release" / "lib");
    }

    return roots;
}

std::filesystem::path findUnixLibgccDir()
{
#if !defined(_WIN32)
    static std::filesystem::path cached = []() -> std::filesystem::path {
        FILE *pipe = EZ_POPEN("cc -print-libgcc-file-name 2>/dev/null", "r");
        if (!pipe)
            pipe = EZ_POPEN("gcc -print-libgcc-file-name 2>/dev/null", "r");
        if (pipe)
        {
            char buf[512];
            std::string out;
            if (fgets(buf, sizeof(buf), pipe) != nullptr)
            {
                out = buf;
                while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
                    out.pop_back();
            }
            EZ_PCLOSE(pipe);
            std::error_code ec;
            if (!out.empty() && std::filesystem::exists(out, ec))
            {
                return std::filesystem::path(out).parent_path();
            }
        }
        return {};
    }();
    return cached;
#else
    return {};
#endif
}

void appendUnixLibraryArgs(const std::filesystem::path &libPath,
                           const std::string &fallbackName,
                           std::vector<std::string> &args)
{
    if (!libPath.empty())
    {
        if (libPath.has_parent_path())
        {
            args.push_back("-L" + libPath.parent_path().string());
        }
        std::string stem = libPath.stem().string();
        if (stem.starts_with("lib"))
        {
            args.push_back("-l" + stem.substr(3));
        }
        else
        {
            args.push_back("-l:" + libPath.filename().string());
        }
    }
    else
    {
        args.push_back("-l" + fallbackName);
    }
}

void appendWindowsImportLibPaths(const std::filesystem::path &linkerPath, std::vector<std::string> &args)
{
    std::error_code ec;
    std::filesystem::path binDir = linkerPath.parent_path();
    std::vector<std::filesystem::path> mingwCandidates = {
        binDir / ".." / "x86_64-w64-mingw32" / "lib",
        binDir / ".." / "lib"
    };
    for (const auto &cand : mingwCandidates)
    {
        if (std::filesystem::exists(cand / "libkernel32.a", ec))
        {
            args.push_back("-L" + std::filesystem::weakly_canonical(cand, ec).string());
            return;
        }
    }
    for (const auto &sdkDir : SystemLinkerDetector::getKnownMsvcLibPaths())
    {
        if (std::filesystem::exists(sdkDir / "kernel32.lib", ec))
        {
            args.push_back("-L" + sdkDir.string());
        }
    }
}

} // namespace

std::filesystem::path EzLinkerDriver::findMuslLibrary(const std::string &targetTriple)
{
    const char *envPath = std::getenv("EZPACKER_MUSL_PATH");
    if (envPath && *envPath)
    {
        std::filesystem::path p(envPath);
        std::error_code ec;
        if (std::filesystem::exists(p, ec))
            return std::filesystem::weakly_canonical(p, ec);
    }

    auto searchRoots = buildLibrarySearchRoots();
    std::vector<std::string> libNames;
    if (isWindowsTargetTriple(targetTriple))
    {
        libNames = { "EzMusl.lib", "libEzMusl.a", "musl.lib", "libmusl.a", "libc.a" };
    }
    else
    {
        libNames = { "libEzMusl.a", "libmusl.a", "libc.a", "EzMusl.lib", "musl.lib" };
    }

    for (const auto &root : searchRoots)
    {
        for (const auto &name : libNames)
        {
            std::filesystem::path candidate = root / name;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec))
            {
                return std::filesystem::weakly_canonical(candidate, ec);
            }
        }
    }

    return {};
}

std::filesystem::path EzLinkerDriver::findMuslCrt1Object(const std::filesystem::path &muslLibPath)
{
    const char *envPath = std::getenv("EZPACKER_MUSL_CRT1_PATH");
    if (envPath && *envPath)
    {
        std::filesystem::path p(envPath);
        std::error_code ec;
        if (std::filesystem::exists(p, ec))
            return std::filesystem::weakly_canonical(p, ec);
    }

    if (!muslLibPath.empty() && muslLibPath.has_parent_path())
    {
        std::filesystem::path nextToLib = muslLibPath.parent_path() / "crt1.o";
        std::error_code ec;
        if (std::filesystem::exists(nextToLib, ec))
            return std::filesystem::weakly_canonical(nextToLib, ec);
    }

    auto searchRoots = buildLibrarySearchRoots();
    for (const auto &root : searchRoots)
    {
        std::filesystem::path candidate = root / "crt1.o";
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec))
        {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }

    return {};
}

std::filesystem::path EzLinkerDriver::findEzExceptionRuntimeLibrary(const std::string &targetTriple)
{
    (void)targetTriple;
    auto searchRoots = buildLibrarySearchRoots();

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

    std::string outPath = opts.outputFile.empty() ? "a.exe" : opts.outputFile;
    args.push_back("/out:" + outPath);

    for (const auto &in : opts.inputFiles)
    {
        args.push_back(in);
    }

    for (const auto &p : opts.libSearchPaths)
    {
        args.push_back("/libpath:" + p);
    }

    for (const auto &lib : opts.libraries)
    {
        std::string libName = lib;
        if (!libName.ends_with(".lib"))
        {
            libName += ".lib";
        }
        args.push_back(libName);
    }

    if (!opts.noDefaultLibs)
    {
        auto msvcPaths = SystemLinkerDetector::getKnownMsvcLibPaths();
        for (const auto &mp : msvcPaths)
        {
            args.push_back("/libpath:" + mp.string());
        }

        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
    }

    // Hosted musl C standard library (EzMusl)
    if (opts.isHosted && !opts.noDefaultLibs)
    {
        std::filesystem::path muslPath = opts.muslPath;
        if (muslPath.empty())
        {
            muslPath = findMuslLibrary(opts.targetTriple);
        }

        if (!muslPath.empty())
        {
            if (muslPath.has_parent_path())
            {
                args.push_back("/libpath:" + muslPath.parent_path().string());
            }
            args.push_back(muslPath.filename().string());
        }
        else
        {
            args.push_back("EzMusl.lib");
        }
    }

    if (!opts.noCompilerRt)
    {
        auto rtPaths = SystemLinkerDetector::getKnownCompilerRtPaths();
        for (const auto &rt : rtPaths)
        {
            args.push_back("/libpath:" + rt.string());
        }
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
                args.push_back("/libpath:" + exRtPath.parent_path().string());
            }
            args.push_back(exRtPath.filename().string());
        }
        else
        {
            args.push_back("EzExceptionRuntime.lib");
        }
    }

    if (!opts.entryPoint.empty())
    {
        args.push_back("/entry:" + opts.entryPoint);
    }
    else if (opts.isHosted && !opts.isShared)
    {
        args.push_back("/entry:mainCRTStartup");
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

    for (const auto &raw : opts.rawLinkerArgs)
    {
        args.push_back(raw);
    }
}

void EzLinkerDriver::appendElfArgs(const LinkerOptions &opts,
                                   const std::filesystem::path &linkerPath,
                                   std::vector<std::string> &args) const
{
    const bool winTarget = isWindowsTargetTriple(opts.targetTriple);
    std::filesystem::path muslPath;
    if (opts.isHosted && !opts.noDefaultLibs)
    {
        muslPath = opts.muslPath.empty() ? findMuslLibrary(opts.targetTriple) : opts.muslPath;
    }

    std::string outPath = opts.outputFile.empty() ? (winTarget ? "a.exe" : "a.out") : opts.outputFile;
    args.push_back("-o");
    args.push_back(outPath);

    if (opts.isHosted && !opts.noDefaultLibs && !winTarget && !opts.isShared)
    {
        std::filesystem::path crt1Path = findMuslCrt1Object(muslPath);
        if (!crt1Path.empty())
        {
            args.push_back(crt1Path.string());
        }
    }

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

    // Hosted musl C standard library (EzMusl) without host -lc
    if (opts.isHosted && !opts.noDefaultLibs)
    {
        appendUnixLibraryArgs(muslPath, "EzMusl", args);
        if (winTarget)
        {
            appendWindowsImportLibPaths(linkerPath, args);
            args.push_back("-lkernel32");
        }
    }

    if (!opts.noCompilerRt && !winTarget)
    {
        std::filesystem::path libgccDir = findUnixLibgccDir();
        if (!libgccDir.empty())
        {
            args.push_back("-L" + libgccDir.string());
        }
        args.push_back("-lgcc");
    }

    if (!opts.noExceptionRuntime)
    {
        std::filesystem::path exRtPath = opts.exceptionRuntimePath;
        if (exRtPath.empty())
        {
            exRtPath = findEzExceptionRuntimeLibrary(opts.targetTriple);
        }
        appendUnixLibraryArgs(exRtPath, "EzExceptionRuntime", args);
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
    else if (opts.isHosted && !opts.isShared && winTarget)
    {
        args.push_back("-e");
        args.push_back("mainCRTStartup");
    }

    for (const auto &raw : opts.rawLinkerArgs)
    {
        args.push_back(raw);
    }
}

void EzLinkerDriver::appendDriverArgs(const LinkerOptions &opts, std::vector<std::string> &args) const
{
    const bool winTarget = isWindowsTargetTriple(opts.targetTriple);
    std::filesystem::path muslPath;
    if (opts.isHosted && !opts.noDefaultLibs)
    {
        muslPath = opts.muslPath.empty() ? findMuslLibrary(opts.targetTriple) : opts.muslPath;
    }

    std::string outPath = opts.outputFile.empty() ? (winTarget ? "a.exe" : "a.out") : opts.outputFile;
    args.push_back("-o");
    args.push_back(outPath);

    if (opts.isHosted && !opts.noDefaultLibs)
    {
        args.push_back("-nostdlib");
        if (!winTarget && !opts.isShared)
        {
            std::filesystem::path crt1Path = findMuslCrt1Object(muslPath);
            if (!crt1Path.empty())
            {
                args.push_back(crt1Path.string());
            }
        }
    }

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

    if (opts.isHosted && !opts.noDefaultLibs)
    {
        appendUnixLibraryArgs(muslPath, "EzMusl", args);
        if (winTarget)
        {
            args.push_back("-lkernel32");
        }
        else if (!opts.noCompilerRt)
        {
            args.push_back("-lgcc");
        }
    }

    if (!opts.noExceptionRuntime)
    {
        std::filesystem::path exRtPath = opts.exceptionRuntimePath;
        if (exRtPath.empty())
        {
            exRtPath = findEzExceptionRuntimeLibrary(opts.targetTriple);
        }
        appendUnixLibraryArgs(exRtPath, "EzExceptionRuntime", args);
    }

    if (!opts.entryPoint.empty())
    {
        args.push_back("-Wl,-e," + opts.entryPoint);
    }
    else if (opts.isHosted && !opts.isShared && winTarget)
    {
        args.push_back("-Wl,-e,mainCRTStartup");
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
        appendElfArgs(opts, detectedLinker.path, args);
    }
    else if (detectedLinker.isCompilerDriver())
    {
        appendDriverArgs(opts, args);
    }
    else
    {
        appendElfArgs(opts, detectedLinker.path, args);
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

#if defined(_WIN32)
    std::string runCmd = "call " + res.commandLine + " 2>&1";
#else
    std::string runCmd = res.commandLine + " 2>&1";
#endif
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

    int rawStatus = EZ_PCLOSE(pipe);
#if defined(_WIN32)
    res.exitCode = rawStatus;
#else
    if (rawStatus == -1)
    {
        res.exitCode = 1;
    }
    else if (WIFEXITED(rawStatus))
    {
        res.exitCode = WEXITSTATUS(rawStatus);
    }
    else if (WIFSIGNALED(rawStatus))
    {
        res.exitCode = 128 + WTERMSIG(rawStatus);
    }
    else
    {
        res.exitCode = 1;
    }
#endif
    res.success = (res.exitCode == 0);

    return res;
}

} // namespace EzLinker
