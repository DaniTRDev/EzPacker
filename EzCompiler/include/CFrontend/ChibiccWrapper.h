#ifndef EZPACKER_CHIBICC_WRAPPER_H
#define EZPACKER_CHIBICC_WRAPPER_H

#include "EzCompilerCommon.h"
#include <string_view>
#include <string>
#include <vector>

// Forward declaration of C AST types to keep chibicc.h internal where possible
struct Obj;
struct Node;
struct Type;
struct Token;

namespace EzCompiler::CFrontend
{

/**
 * Result of invoking chibicc's preprocessor and parser on C source.
 */
struct ChibiccParseResult
{
    bool success{ false };
    std::string errorMessage;
    struct Obj *astRoot{ nullptr }; // Linked list of Obj (functions and globals)
};

/**
 * Wrapper class managing the execution lifecycle, error trapping, and state
 * reset for the embedded chibicc compiler frontend.
 */
class ChibiccWrapper
{
  public:
    ChibiccWrapper();
    ~ChibiccWrapper();

    /**
     * Resolves the disk path to the bundled ISO C11 freestanding headers directory.
     * Returns empty string if not found on disk.
     */
    static std::string getBundledFreestandingIncludeDir();

    /**
     * Adds an include directory for header resolution (<...> and "...").
     */
    void addIncludePath(std::string_view path);

    /**
     * Defines a preprocessor macro (e.g. "-DDEBUG" or "-DMAX=100").
     */
    void defineMacro(std::string_view name, std::string_view value = "1");

    /**
     * Parses a C source file at the specified file path into a chibicc AST.
     * Uses error interception (setjmp/longjmp) to guarantee no process termination.
     */
    ChibiccParseResult parseFile(std::string_view filePath);

    /**
     * Parses C source code from an in-memory string buffer.
     * Creates a temporary source file and returns the parsed AST.
     */
    ChibiccParseResult parseString(std::string_view sourceCode, std::string_view virtualFileName = "input.c");

  private:
    void setupDefaultMacros();

  private:
    std::vector<std::string> m_includePaths;
    std::vector<std::pair<std::string, std::string>> m_macros;
};

} // namespace EzCompiler::CFrontend

#endif // EZPACKER_CHIBICC_WRAPPER_H
