#ifndef EZDSL_CODE_GENERATOR_H
#define EZDSL_CODE_GENERATOR_H

#include "CodeGenerators/CppSourceEmitter.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "EzDslCodeGeneratorsCommon.h"
#include "Sema/SymbolTable.h"

namespace CodeGenerators
{

/**
 * Base abstraction for all EzDSL code generation passes.
 * Encapsulates the SymbolTable, DiagnosticCollector, destination filesystem path,
 * standardized path normalization, and atomic timestamp-preserving file output.
 */
class CodeGenerator
{
  public:
    /**
     * Pair of resolved output paths for generators that emit a C++ header/source couple.
     */
    struct HeaderAndSourcePaths
    {
        std::filesystem::path m_headerPath; ///< Resolved destination of the generated header.
        std::filesystem::path m_sourcePath; ///< Resolved destination of the generated source.
    };

  public:
    CodeGenerator(std::string_view generatorName,
                  DiagnosticCollector *collector,
                  SymbolTable *table,
                  std::filesystem::path outPath);
    virtual ~CodeGenerator() = default;

    /**
     * Executes the code generation pass.
     * @return true on success, false if an error occurred during generation.
     */
    virtual bool run() = 0;

    // --- Accessors ---

    std::string_view getGeneratorName() const noexcept { return m_generatorName; }
    DiagnosticCollector *getCollector() const noexcept { return m_collector; }
    SymbolTable *getSymbolTable() const noexcept { return m_table; }
    const std::filesystem::path &getOutputPath() const noexcept { return m_outputPath; }

    // --- Atomic File Writing Utility ---

    /**
     * Writes content to filePath only if the file does not exist or has different content.
     * Preserves modification timestamps on unchanged files to avoid triggering redundant CMake rebuild cascades.
     */
    static bool WriteFileIfChanged(const std::filesystem::path &filePath,
                                   std::string_view newContent,
                                   std::string *errorOut = nullptr);

  protected:
    /**
     * Validates that the generator's collector and symbol table pointers are non-null and the output path is non-empty.
     */
    bool validate() const;

    /**
     * Resolves a single file path from m_outputPath.
     * If m_outputPath is a directory or has no extension, appends defaultFileName.
     */
    std::filesystem::path resolveSingleFilePath(std::string_view defaultFileName) const;

    /**
     * Resolves header and source file paths from m_outputPath.
     * If m_outputPath is a directory or has no extension, produces <dir>/<defaultBaseName>.h and .cpp.
     * If m_outputPath has .h/.hpp extension, sets header to it and replaces extension with .cpp for source.
     * If m_outputPath has .cpp/.cxx extension, sets source to it and replaces extension with .h for header.
     */
    HeaderAndSourcePaths resolveHeaderAndSourcePaths(std::string_view defaultBaseName) const;

    /**
     * Writes content to filePath with automated directory creation, timestamp preservation, and diagnostic tracing.
     */
    bool writeOutput(const std::filesystem::path &filePath, std::string_view content) const;

    // --- Diagnostic Logging Helpers ---

    /** Forwards a trace-level diagnostic to the collector, prefixed with the generator name. */
    template <typename... Args> void trace(std::format_string<Args...> fmt, Args &&...args) const
    {
        if (m_collector)
        {
            m_collector->trace(m_generatorName, fmt, std::forward<Args>(args)...);
        }
    }

    /** Forwards an error-level diagnostic to the collector, prefixed with the generator name. */
    template <typename... Args> void error(std::format_string<Args...> fmt, Args &&...args) const
    {
        if (m_collector)
        {
            m_collector->error(m_generatorName, fmt, std::forward<Args>(args)...);
        }
    }

    /** Forwards a warning-level diagnostic to the collector, prefixed with the generator name. */
    template <typename... Args> void warn(std::format_string<Args...> fmt, Args &&...args) const
    {
        if (m_collector)
        {
            m_collector->warn(m_generatorName, fmt, std::forward<Args>(args)...);
        }
    }

  protected:
    std::string m_generatorName;                 ///< Name attached to diagnostics and generated-file banners.
    DiagnosticCollector *m_collector{ nullptr }; ///< Sink for diagnostics; may be null for silent operation.
    SymbolTable *m_table{ nullptr };             ///< Parsed DSL symbols consumed by the generator.
    std::filesystem::path m_outputPath;          ///< Destination file or directory for generated artifacts.
};

} // namespace CodeGenerators

#endif // EZDSL_CODE_GENERATOR_H
