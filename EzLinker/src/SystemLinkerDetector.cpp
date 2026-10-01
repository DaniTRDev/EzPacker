#include "SystemLinkerDetector.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#define EZ_POPEN _popen
#define EZ_PCLOSE _pclose
#else
#include <unistd.h>
#define EZ_POPEN popen
#define EZ_PCLOSE pclose
#endif

namespace EzLinker
{

std::string_view DetectedLinker::getKindName() const
{
    switch (kind)
    {
    case SystemLinkerKind::LldLink:     return "lld-link";
    case SystemLinkerKind::MsvcLink:    return "link.exe (MSVC)";
    case SystemLinkerKind::LldElf:      return "ld.lld";
    case SystemLinkerKind::GnuBfd:      return "ld.bfd / ld";
    case SystemLinkerKind::GnuGold:     return "ld.gold";
    case SystemLinkerKind::ClangDriver: return "clang";
    case SystemLinkerKind::GccDriver:   return "gcc";
    default:                            return "Unknown";
    }
}

SystemLinkerKind SystemLinkerDetector::classifyLinker(const std::filesystem::path &path)
{
    std::string filename = path.filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (filename.find("lld-link") != std::string::npos)
    {
        return SystemLinkerKind::LldLink;
    }
    if (filename.find("ld.lld") != std::string::npos)
    {
        return SystemLinkerKind::LldElf;
    }
    if (filename.find("ld.gold") != std::string::npos)
    {
        return SystemLinkerKind::GnuGold;
    }
    if (filename.find("ld.bfd") != std::string::npos)
    {
        return SystemLinkerKind::GnuBfd;
    }
    if (filename.find("clang") != std::string::npos)
    {
        return SystemLinkerKind::ClangDriver;
    }
    if (filename.find("gcc") != std::string::npos || filename.find("g++") != std::string::npos ||
        filename.find("c++") != std::string::npos)
    {
        return SystemLinkerKind::GccDriver;
    }
    if (filename == "link" || filename == "link.exe")
    {
        return SystemLinkerKind::MsvcLink;
    }
    if (filename == "ld" || filename == "ld.exe")
    {
#if defined(_WIN32)
        // MinGW ld.exe behaves as a GNU BFD/ELF/PE linker
        return SystemLinkerKind::GnuBfd;
#else
        return SystemLinkerKind::GnuBfd;
#endif
    }

    return SystemLinkerKind::Unknown;
}

std::vector<std::filesystem::path> SystemLinkerDetector::getSystemSearchPaths()
{
    std::vector<std::filesystem::path> paths;

    const char *pathEnv = std::getenv("PATH");
    if (pathEnv)
    {
#if defined(_WIN32)
        char delimiter = ';';
#else
        char delimiter = ':';
#endif
        std::string envStr(pathEnv);
        size_t start = 0;
        size_t end = envStr.find(delimiter);
        while (end != std::string::npos)
        {
            std::string token = envStr.substr(start, end - start);
            if (!token.empty())
            {
                paths.emplace_back(token);
            }
            start = end + 1;
            end = envStr.find(delimiter, start);
        }
        if (start < envStr.length())
        {
            paths.emplace_back(envStr.substr(start));
        }
    }

#if defined(_WIN32)
    // Standard LLVM Windows locations
    paths.emplace_back("C:\\Program Files\\LLVM\\bin");
    paths.emplace_back("C:\\LLVM\\bin");

    // Standard MinGW locations
    paths.emplace_back("C:\\msys64\\mingw64\\bin");
    paths.emplace_back("C:\\msys64\\ucrt64\\bin");

    // Visual Studio toolchain search
    const char *vsInstall = std::getenv("VSINSTALLDIR");
    if (vsInstall)
    {
        paths.emplace_back(std::filesystem::path(vsInstall) / "VC" / "Tools" / "MSVC");
    }

    // Common MSVC install paths
    std::vector<std::string> vsEditions = { "Community", "Professional", "Enterprise", "BuildTools" };
    for (const auto &ed : vsEditions)
    {
        std::filesystem::path msvcBase = "C:\\Program Files\\Microsoft Visual Studio\\2022\\" + ed + "\\VC\\Tools\\MSVC";
        if (std::filesystem::exists(msvcBase))
        {
            std::error_code ec;
            for (const auto &entry : std::filesystem::directory_iterator(msvcBase, ec))
            {
                if (entry.is_directory())
                {
                    paths.push_back(entry.path() / "bin" / "Hostx64" / "x64");
                    paths.push_back(entry.path() / "bin" / "HostX64" / "x64");
                }
            }
        }
    }
#else
    // Standard Unix / Linux locations
    paths.emplace_back("/usr/local/bin");
    paths.emplace_back("/usr/bin");
    paths.emplace_back("/bin");
    paths.emplace_back("/opt/llvm/bin");
#endif

    return paths;
}

std::filesystem::path SystemLinkerDetector::findExecutableOnPath(const std::string &name)
{
    // If name is already an absolute path and exists, return it directly
    std::filesystem::path directPath(name);
    if (directPath.is_absolute() && std::filesystem::exists(directPath))
    {
        return directPath;
    }

    auto searchPaths = getSystemSearchPaths();
    std::vector<std::string> candidateNames = { name };

#if defined(_WIN32)
    if (!name.ends_with(".exe"))
    {
        candidateNames.push_back(name + ".exe");
    }
#endif

    for (const auto &dir : searchPaths)
    {
        for (const auto &cand : candidateNames)
        {
            std::filesystem::path full = dir / cand;
            std::error_code ec;
            if (std::filesystem::exists(full, ec) && !std::filesystem::is_directory(full, ec))
            {
                return full;
            }
        }
    }

    return {};
}

std::vector<std::filesystem::path> SystemLinkerDetector::getKnownCompilerRtPaths()
{
    std::vector<std::filesystem::path> rtPaths;

    const char *llvmRoot = std::getenv("LLVM_DIR");
    if (llvmRoot)
    {
        rtPaths.emplace_back(std::filesystem::path(llvmRoot) / "lib" / "clang");
    }

#if defined(_WIN32)
    std::filesystem::path defaultLlvm = "C:\\Program Files\\LLVM\\lib\\clang";
    if (std::filesystem::exists(defaultLlvm))
    {
        rtPaths.push_back(defaultLlvm);
    }
#else
    std::filesystem::path defaultLinuxLlvm = "/usr/lib/llvm/lib/clang";
    if (std::filesystem::exists(defaultLinuxLlvm))
    {
        rtPaths.push_back(defaultLinuxLlvm);
    }
#endif

    return rtPaths;
}

std::vector<std::filesystem::path> SystemLinkerDetector::getKnownMsvcLibPaths()
{
    std::vector<std::filesystem::path> libPaths;

    const char *libEnv = std::getenv("LIB");
    if (libEnv)
    {
        std::string envStr(libEnv);
        size_t start = 0;
        size_t end = envStr.find(';');
        while (end != std::string::npos)
        {
            std::string token = envStr.substr(start, end - start);
            if (!token.empty() && std::filesystem::exists(token))
            {
                libPaths.emplace_back(token);
            }
            start = end + 1;
            end = envStr.find(';', start);
        }
        if (start < envStr.length())
        {
            std::string token = envStr.substr(start);
            if (!token.empty() && std::filesystem::exists(token))
            {
                libPaths.emplace_back(token);
            }
        }
    }

#if defined(_WIN32)
    // Check known MSVC 2022 directories if LIB env var is empty
    if (libPaths.empty())
    {
        std::vector<std::string> vsEditions = { "Community", "Professional", "Enterprise", "BuildTools" };
        for (const auto &ed : vsEditions)
        {
            std::filesystem::path msvcBase = "C:\\Program Files\\Microsoft Visual Studio\\2022\\" + ed + "\\VC\\Tools\\MSVC";
            if (std::filesystem::exists(msvcBase))
            {
                std::error_code ec;
                for (const auto &entry : std::filesystem::directory_iterator(msvcBase, ec))
                {
                    if (entry.is_directory())
                    {
                        std::filesystem::path libX64 = entry.path() / "lib" / "x64";
                        if (std::filesystem::exists(libX64))
                        {
                            libPaths.push_back(libX64);
                            break;
                        }
                    }
                }
                if (!libPaths.empty())
                {
                    break;
                }
            }
        }

        // Check Windows SDK paths for ucrt and um
        std::filesystem::path sdkBase = "C:\\Program Files (x86)\\Windows Kits\\10\\Lib";
        if (std::filesystem::exists(sdkBase))
        {
            std::error_code ec;
            std::filesystem::path latestVersion;
            for (const auto &entry : std::filesystem::directory_iterator(sdkBase, ec))
            {
                if (entry.is_directory() && entry.path().filename().string().starts_with("10."))
                {
                    latestVersion = entry.path();
                }
            }
            if (!latestVersion.empty())
            {
                std::filesystem::path ucrtPath = latestVersion / "ucrt" / "x64";
                std::filesystem::path umPath = latestVersion / "um" / "x64";
                if (std::filesystem::exists(ucrtPath)) libPaths.push_back(ucrtPath);
                if (std::filesystem::exists(umPath)) libPaths.push_back(umPath);
            }
        }
    }
#endif

    return libPaths;
}

std::string SystemLinkerDetector::queryVersion(const std::filesystem::path &linkerPath, SystemLinkerKind kind)
{
    std::string flag = "--version";
    if (kind == SystemLinkerKind::MsvcLink)
    {
        flag = ""; // link.exe prints banner with no args or /?
    }
    else if (kind == SystemLinkerKind::LldLink)
    {
        flag = "--version";
    }

    std::string cmd = "\"" + linkerPath.string() + "\" " + flag + " 2>&1";
    FILE *pipe = EZ_POPEN(cmd.c_str(), "r");
    if (!pipe)
    {
        return "";
    }

    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result = buffer;
        // Strip trailing newline
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
        {
            result.pop_back();
        }
    }
    EZ_PCLOSE(pipe);
    return result;
}

DetectedLinker SystemLinkerDetector::detectForWindows(const std::string &targetTriple)
{
    // Preferred order: lld-link -> link.exe -> ld.lld -> ld (MinGW) -> clang -> gcc
    std::vector<std::string> candidates = {
        "lld-link",
        "link",
        "ld.lld",
        "ld",
        "clang",
        "gcc"
    };

    for (const auto &cand : candidates)
    {
        auto found = findExecutableOnPath(cand);
        if (!found.empty())
        {
            DetectedLinker dl;
            dl.path = found;
            dl.kind = classifyLinker(found);
            dl.version = queryVersion(found, dl.kind);
            return dl;
        }
    }

    return {};
}

DetectedLinker SystemLinkerDetector::detectForLinux(const std::string &targetTriple)
{
    // Preferred order: ld.lld -> ld.gold -> ld.bfd -> ld -> clang -> gcc
    std::vector<std::string> candidates = {
        "ld.lld",
        "ld.gold",
        "ld.bfd",
        "ld",
        "clang",
        "gcc"
    };

    for (const auto &cand : candidates)
    {
        auto found = findExecutableOnPath(cand);
        if (!found.empty())
        {
            DetectedLinker dl;
            dl.path = found;
            dl.kind = classifyLinker(found);
            dl.version = queryVersion(found, dl.kind);
            return dl;
        }
    }

    return {};
}

DetectedLinker SystemLinkerDetector::detect(const std::string &userLinkerPath, const std::string &targetTriple)
{
    if (!userLinkerPath.empty())
    {
        std::filesystem::path p(userLinkerPath);
        if (!p.is_absolute())
        {
            p = findExecutableOnPath(userLinkerPath);
        }

        if (std::filesystem::exists(p))
        {
            DetectedLinker dl;
            dl.path = p;
            dl.kind = classifyLinker(p);
            dl.version = queryVersion(p, dl.kind);
            return dl;
        }
    }

    bool isWindowsTarget = false;
    if (!targetTriple.empty())
    {
        std::string lowerTriple = targetTriple;
        std::transform(lowerTriple.begin(), lowerTriple.end(), lowerTriple.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lowerTriple.find("windows") != std::string::npos || lowerTriple.find("win32") != std::string::npos)
        {
            isWindowsTarget = true;
        }
    }
    else
    {
#if defined(_WIN32)
        isWindowsTarget = true;
#endif
    }

    if (isWindowsTarget)
    {
        return detectForWindows(targetTriple);
    }
    return detectForLinux(targetTriple);
}

} // namespace EzLinker
