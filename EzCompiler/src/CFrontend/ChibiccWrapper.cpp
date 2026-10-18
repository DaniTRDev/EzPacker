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
#endif

#include "chibicc.h"

namespace EzCompiler::CFrontend
{

ChibiccWrapper::ChibiccWrapper()
{
    setupDefaultMacros();
}

ChibiccWrapper::~ChibiccWrapper() = default;

std::string ChibiccWrapper::getBundledFreestandingIncludeDir()
{
    // 1. Environment variable override
    const char *envDir = std::getenv("EZPACKER_FREESTANDING_DIR");
    if (envDir && *envDir)
    {
        std::filesystem::path p(envDir);
        std::error_code ec;
        if (std::filesystem::is_directory(p, ec))
            return std::filesystem::weakly_canonical(p, ec).string();
    }

    // 2. Compile-time definition
#ifdef EZPACKER_FREESTANDING_INCLUDE_DIR
    {
        std::filesystem::path p(EZPACKER_FREESTANDING_INCLUDE_DIR);
        std::error_code ec;
        if (std::filesystem::is_directory(p, ec) && std::filesystem::exists(p / "stdint.h", ec))
            return std::filesystem::weakly_canonical(p, ec).string();
    }
#endif

    // 3. Executable-relative resolution
#if defined(_WIN32)
    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        std::vector<std::filesystem::path> candidates = {
            exeDir / "include" / "freestanding",
            exeDir / ".." / "include" / "freestanding",
            exeDir / ".." / ".." / "include" / "freestanding",
            exeDir / ".." / ".." / ".." / "include" / "freestanding"
        };
        for (const auto &cand : candidates)
        {
            std::error_code ec;
            if (std::filesystem::is_directory(cand, ec) && std::filesystem::exists(cand / "stdint.h", ec))
                return std::filesystem::weakly_canonical(cand, ec).string();
        }
    }
#endif

    // 4. Working directory / relative paths
    std::error_code cwdEc;
    std::filesystem::path cwd = std::filesystem::current_path(cwdEc);
    if (!cwdEc)
    {
        std::vector<std::filesystem::path> candidates = {
            cwd / "include" / "freestanding",
            cwd / "bin" / "include" / "freestanding",
            cwd / ".." / "include" / "freestanding",
            cwd / ".." / ".." / "include" / "freestanding"
        };
        for (const auto &cand : candidates)
        {
            std::error_code ec;
            if (std::filesystem::is_directory(cand, ec) && std::filesystem::exists(cand / "stdint.h", ec))
                return std::filesystem::weakly_canonical(cand, ec).string();
        }
    }

    return {};
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
    defineMacro("__chibicc__", "1");
    defineMacro("__STDC__", "1");
    defineMacro("__STDC_VERSION__", "201112L");
    defineMacro("__STDC_HOSTED__", "0");
    defineMacro("__EZPACKER_STANDALONE__", "1");
    defineMacro("__STDC_FREESTANDING__", "1");
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

    include_paths.data = nullptr;
    include_paths.capacity = 0;
    include_paths.len = 0;

    for (const auto &inc : m_includePaths)
    {
        strarray_push(&include_paths, const_cast<char *>(inc.c_str()));
    }

    std::string fsDir = getBundledFreestandingIncludeDir();
    if (!fsDir.empty())
    {
        bool found = false;
        for (const auto &inc : m_includePaths)
        {
            if (inc == fsDir)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            strarray_push(&include_paths, const_cast<char *>(fsDir.c_str()));
        }
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
