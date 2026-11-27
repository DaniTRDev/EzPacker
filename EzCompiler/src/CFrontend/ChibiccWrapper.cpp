#include "CFrontend/ChibiccWrapper.h"
#include <csetjmp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "chibicc.h"

namespace EzCompiler::CFrontend
{

namespace
{

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

std::string resolveIncludeDir(const char *envVarName,
                              const char *compileTimePath,
                              const char *sentinelRelFile,
                              const std::string &subDirName,
                              const std::vector<std::filesystem::path> &extraFallbacks = {})
{
    if (envVarName)
    {
        const char *envDir = std::getenv(envVarName);
        if (envDir && *envDir)
        {
            std::filesystem::path p(envDir);
            std::error_code ec;
            if (std::filesystem::is_directory(p, ec))
                return std::filesystem::weakly_canonical(p, ec).string();
        }
    }

    auto hasSentinel = [&](const std::filesystem::path &dir) -> bool {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec))
            return false;
        if (!sentinelRelFile || !*sentinelRelFile)
            return true;
        return std::filesystem::exists(dir / sentinelRelFile, ec);
    };

    if (compileTimePath && *compileTimePath)
    {
        std::filesystem::path p(compileTimePath);
        if (hasSentinel(p))
        {
            std::error_code ec;
            return std::filesystem::weakly_canonical(p, ec).string();
        }
    }

    std::filesystem::path exeDir = getCurrentExecutableDir();
    if (!exeDir.empty())
    {
        std::vector<std::filesystem::path> candidates = {
            exeDir / "include" / subDirName,
            exeDir / ".." / "include" / subDirName,
            exeDir / ".." / ".." / "include" / subDirName,
            exeDir / ".." / ".." / ".." / "include" / subDirName,
        };
        for (const auto &extra : extraFallbacks)
        {
            candidates.push_back(exeDir / extra);
        }
        for (const auto &cand : candidates)
        {
            if (hasSentinel(cand))
            {
                std::error_code ec;
                return std::filesystem::weakly_canonical(cand, ec).string();
            }
        }
    }

    std::error_code cwdEc;
    std::filesystem::path cwd = std::filesystem::current_path(cwdEc);
    if (!cwdEc)
    {
        std::vector<std::filesystem::path> candidates = {
            cwd / "include" / subDirName,
            cwd / "bin" / "include" / subDirName,
            cwd / ".." / "include" / subDirName,
            cwd / ".." / ".." / "include" / subDirName,
        };
        for (const auto &extra : extraFallbacks)
        {
            candidates.push_back(cwd / extra);
        }
        for (const auto &cand : candidates)
        {
            if (hasSentinel(cand))
            {
                std::error_code ec;
                return std::filesystem::weakly_canonical(cand, ec).string();
            }
        }
    }

    return {};
}

void pushUniqueIncludePath(const std::string &path)
{
    if (path.empty())
        return;
    for (int i = 0; i < include_paths.len; ++i)
    {
        if (include_paths.data[i] && path == include_paths.data[i])
            return;
    }
    strarray_push(&include_paths, const_cast<char *>(path.c_str()));
}

} // namespace

ChibiccWrapper::ChibiccWrapper()
{
    setupDefaultMacros();
}

ChibiccWrapper::~ChibiccWrapper() = default;

std::string ChibiccWrapper::getBundledFreestandingIncludeDir()
{
#ifdef EZPACKER_FREESTANDING_INCLUDE_DIR
    const char *compiledDir = EZPACKER_FREESTANDING_INCLUDE_DIR;
#else
    const char *compiledDir = nullptr;
#endif
    return resolveIncludeDir("EZPACKER_FREESTANDING_DIR", compiledDir, "stdint.h", "freestanding");
}

std::string ChibiccWrapper::getBundledHostedIncludeDir()
{
#ifdef EZPACKER_MUSL_INCLUDE_DIR
    const char *compiledDir = EZPACKER_MUSL_INCLUDE_DIR;
#else
    const char *compiledDir = nullptr;
#endif
    return resolveIncludeDir("EZPACKER_MUSL_DIR",
                             compiledDir,
                             "bits/alltypes.h",
                             "musl",
                             { std::filesystem::path("EzMusl") / "staged_include",
                               std::filesystem::path("build") / "bin" / "include" / "musl" });
}

std::string ChibiccWrapper::getBundledHostedCompatIncludeDir()
{
#ifdef EZPACKER_MUSL_COMPAT_INCLUDE_DIR
    const char *compiledDir = EZPACKER_MUSL_COMPAT_INCLUDE_DIR;
#else
    const char *compiledDir = nullptr;
#endif
    return resolveIncludeDir("EZPACKER_MUSL_COMPAT_DIR",
                             compiledDir,
                             "stdarg.h",
                             "musl-compat",
                             { std::filesystem::path("EzMusl") / "compat_include",
                               std::filesystem::path("..") / "EzMusl" / "compat_include" });
}

void ChibiccWrapper::setTargetTriple(const TargetTriple &triple)
{
    m_target = triple;
}

void ChibiccWrapper::setHostedMode(bool hosted)
{
    m_hosted = hosted;
    setupDefaultMacros();
}

void ChibiccWrapper::addIncludePath(std::string_view path)
{
    for (const auto &inc : m_includePaths)
    {
        if (inc == path)
            return;
    }
    m_includePaths.emplace_back(path);
}

void ChibiccWrapper::defineMacro(std::string_view name, std::string_view value)
{
    m_macros.emplace_back(std::string(name), std::string(value));
}

void ChibiccWrapper::setupDefaultMacros()
{
    m_macros.clear();
    defineMacro("__chibicc__", "1");
    defineMacro("__STDC__", "1");
    defineMacro("__STDC_VERSION__", "201112L");
    if (m_hosted)
    {
        defineMacro("__STDC_HOSTED__", "1");
        defineMacro("__EZPACKER_HOSTED__", "1");
    }
    else
    {
        defineMacro("__STDC_HOSTED__", "0");
        defineMacro("__EZPACKER_STANDALONE__", "1");
        defineMacro("__STDC_FREESTANDING__", "1");
    }
    defineMacro("__builtin_alloca", "alloca");
    defineMacro("__asm__", "asm");
    defineMacro("__asm", "asm");
    defineMacro("__volatile", "volatile");
    defineMacro("__volatile__", "volatile");
    defineMacro("__inline", "inline");
    defineMacro("__inline__", "inline");
    defineMacro("__const", "const");
    defineMacro("__const__", "const");
    defineMacro("__restrict", "restrict");
    defineMacro("__restrict__", "restrict");
    defineMacro("__extension__", "");
    defineMacro("__attribute__(x)", "");
    defineMacro("__builtin_expect(x, v)", "(x)");
    defineMacro("__builtin_constant_p(x)", "0");
}

ChibiccParseResult ChibiccWrapper::parseFile(std::string_view filePath)
{
    ChibiccParseResult result;

    std::filesystem::path p(filePath);
    std::error_code ec;
    if (!std::filesystem::exists(p, ec))
    {
        result.success = false;
        result.errorMessage = "File does not exist: " + std::string(filePath);
        return result;
    }

    chibicc_reset_state();
    init_macros();

    // Align predefined target macros with the active TargetTriple while keeping
    // chibicc's internal LP64 data model (_LP64 / __LP64__ / __SIZEOF_LONG__ == 8).
    if (m_target.isRiscV64())
    {
        undef_macro(const_cast<char *>("__amd64"));
        undef_macro(const_cast<char *>("__amd64__"));
        undef_macro(const_cast<char *>("__x86_64"));
        undef_macro(const_cast<char *>("__x86_64__"));
        define_macro(const_cast<char *>("__riscv"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__riscv_xlen"), const_cast<char *>("64"));
    }

    if (m_target.isWindows())
    {
        undef_macro(const_cast<char *>("__ELF__"));
        define_macro(const_cast<char *>("_WIN32"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("_WIN64"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__WIN32__"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__WIN64__"), const_cast<char *>("1"));
    }
    else if (m_target.isLinux())
    {
        define_macro(const_cast<char *>("__ELF__"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__linux__"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__linux"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__gnu_linux__"), const_cast<char *>("1"));
    }

    if (m_hosted)
    {
        undef_macro(const_cast<char *>("__EZPACKER_STANDALONE__"));
        undef_macro(const_cast<char *>("__STDC_FREESTANDING__"));
        undef_macro(const_cast<char *>("__STDC_HOSTED__"));
        define_macro(const_cast<char *>("__STDC_HOSTED__"), const_cast<char *>("1"));
        define_macro(const_cast<char *>("__EZPACKER_HOSTED__"), const_cast<char *>("1"));
    }

    include_paths.data = nullptr;
    include_paths.capacity = 0;
    include_paths.len = 0;

    for (const auto &inc : m_includePaths)
    {
        pushUniqueIncludePath(inc);
    }

    if (m_hosted)
    {
        pushUniqueIncludePath(getBundledHostedCompatIncludeDir());
        pushUniqueIncludePath(getBundledHostedIncludeDir());
    }
    else
    {
        pushUniqueIncludePath(getBundledFreestandingIncludeDir());
    }

    for (const auto &[name, val] : m_macros)
    {
        define_macro(const_cast<char *>(name.c_str()), const_cast<char *>(val.c_str()));
    }

    jmp_buf jmpEnv;
    if (setjmp(jmpEnv) != 0)
    {
        result.success = false;
        result.errorMessage = chibicc_get_last_error();
        chibicc_set_error_jmp(nullptr);
        return result;
    }

    chibicc_set_error_jmp(&jmpEnv);

    std::string pathStr = p.string();
    Token *tok = tokenize_file(const_cast<char *>(pathStr.c_str()));
    if (!tok)
    {
        chibicc_set_error_jmp(nullptr);
        result.success = false;
        result.errorMessage = "Failed to tokenize file: " + pathStr;
        return result;
    }

    tok = preprocess(tok);
    Obj *prog = parse(tok);

    chibicc_set_error_jmp(nullptr);

    result.success = true;
    result.astRoot = prog;
    return result;
}

ChibiccParseResult ChibiccWrapper::parseString(std::string_view sourceCode, std::string_view virtualFileName)
{
    std::error_code ec;
    auto tempDir = std::filesystem::temp_directory_path(ec);
    if (ec)
    {
        tempDir = std::filesystem::current_path();
    }
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto tempFile = tempDir / ("ez_c_frontend_" + std::to_string(timestamp) + ".c");

    {
        std::ofstream out(tempFile, std::ios::binary);
        out.write(sourceCode.data(), static_cast<std::streamsize>(sourceCode.size()));
    }

    auto res = parseFile(tempFile.string());
    std::filesystem::remove(tempFile, ec);
    return res;
}

} // namespace EzCompiler::CFrontend
