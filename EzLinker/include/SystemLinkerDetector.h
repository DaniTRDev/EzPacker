#ifndef EZLINKER_SYSTEM_LINKER_DETECTOR_H
#define EZLINKER_SYSTEM_LINKER_DETECTOR_H

#include "EzLinkerCommon.h"

namespace EzLinker
{

/**
 * Enumeration of recognized system linker and compiler driver types.
 */
enum class SystemLinkerKind
{
    Unknown,
    LldLink,     ///< lld-link (LLVM MSVC-compatible linker)
    MsvcLink,    ///< link.exe (Microsoft Visual C++ linker)
    LldElf,      ///< ld.lld (LLVM ELF linker)
    GnuBfd,      ///< ld.bfd / ld (GNU BFD linker)
    GnuGold,     ///< ld.gold (GNU Gold linker)
    ClangDriver, ///< clang / clang++
    GccDriver    ///< gcc / g++
};

/**
 * Result structure of system linker detection.
 */
struct EZLINKER_API DetectedLinker
{
    SystemLinkerKind kind{ SystemLinkerKind::Unknown };
    std::filesystem::path path;
    std::string version;

    [[nodiscard]] bool isValid() const { return kind != SystemLinkerKind::Unknown && !path.empty(); }
    [[nodiscard]] bool isMsvcCompatible() const
    {
        return kind == SystemLinkerKind::LldLink || kind == SystemLinkerKind::MsvcLink;
    }
    [[nodiscard]] bool isElfLinker() const
    {
        return kind == SystemLinkerKind::LldElf || kind == SystemLinkerKind::GnuBfd ||
               kind == SystemLinkerKind::GnuGold;
    }
    [[nodiscard]] bool isCompilerDriver() const
    {
        return kind == SystemLinkerKind::ClangDriver || kind == SystemLinkerKind::GccDriver;
    }
    [[nodiscard]] std::string_view getKindName() const;
};

/**
 * Automated detector for locating and classifying the preferred system linker.
 */
class EZLINKER_API SystemLinkerDetector
{
public:
    /**
     * Resolves the preferred linker on the system or validates a user-provided override.
     */
    static DetectedLinker detect(const std::string &userLinkerPath = "", const std::string &targetTriple = "");

    /**
     * Identifies the linker kind from its file path and binary characteristics.
     */
    static SystemLinkerKind classifyLinker(const std::filesystem::path &path);

    /**
     * Searches PATH and standard toolchain paths for an executable by name.
     */
    static std::filesystem::path findExecutableOnPath(const std::string &name);

    /**
     * Retrieves system search directories derived from PATH and known install locations.
     */
    static std::vector<std::filesystem::path> getSystemSearchPaths();

    /**
     * Searches for compiler-rt builtins library directories on the system.
     */
    static std::vector<std::filesystem::path> getKnownCompilerRtPaths();

    /**
     * Searches for MSVC and Windows SDK standard library paths on Windows.
     */
    static std::vector<std::filesystem::path> getKnownMsvcLibPaths();

private:
    static DetectedLinker detectForWindows(const std::string &targetTriple);
    static DetectedLinker detectForLinux(const std::string &targetTriple);
    static std::string queryVersion(const std::filesystem::path &linkerPath, SystemLinkerKind kind);
};

} // namespace EzLinker

#endif // EZLINKER_SYSTEM_LINKER_DETECTOR_H
