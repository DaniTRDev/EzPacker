#ifndef EZCORE_GENERIC_SOURCE_MANAGER_H
#define EZCORE_GENERIC_SOURCE_MANAGER_H

#include "EzCoreCommon.h"
#include <filesystem>
#include <optional>

/**
 * Lightweight source code span reference linking AST and MIR constructs back to origin files.
 * Stores half-open byte offset interval [begin, end) and an integer file ID resolving through SourceManager.
 */
struct SourceReference
{
    size_t m_beginOffset{ 0 };  // Byte offset of the first character of the span.
    size_t m_endOffset{ 0 };    // Byte offset one past the last character of the span.
    size_t m_sourceFileId{ 0 }; // Numeric ID of the source file this span belongs to.

    /**
     * Returns the span length in bytes (m_endOffset - m_beginOffset).
     */
    size_t length() const { return m_endOffset - m_beginOffset; }
};

/**
 * Represents a precomputed source line mapping byte intervals to 1-based line numbers.
 */
struct SourceLineRange
{
    size_t m_beginOffset{ 0 }; // Byte offset where the line content begins.
    size_t m_endOffset{ 0 };   // Byte offset just past the line content (terminators excluded).
    size_t m_lineNumber{ 0 };  // 1-based line number displayed in diagnostics.

    /**
     * Returns the byte length of this line excluding line terminator characters.
     */
    size_t length() const { return m_endOffset - m_beginOffset; }
};

/**
 * Cached source file descriptor containing raw file content, display name, and precomputed line bounds.
 * Allocated within the compilation arena memory resource.
 */
struct SourceFileEntry
{
    std::pmr::string m_content;                // Full text of the source file, owned by the compilation arena.
    std::pmr::string m_name;                   // Display name/path used to identify the file in diagnostics.
    std::pmr::vector<SourceLineRange> m_lines; // Precomputed line spans, sorted by offset for binary search.
};

/**
 * Abstract interface for source buffer registries, include resolution, and source reference factories.
 * Allows decoupling parsing and diagnostic formatting from concrete file storage engines.
 */
class GenericSourceManager
{
  public:
    virtual ~GenericSourceManager() = default;

    /**
     * Checks if a source buffer with the specified name or canonical path is already registered.
     */
    virtual bool doesSourceNameExist(const std::string_view &sourceName) const = 0;

    /**
     * Ingests in-memory source text under the given identifier name.
     * Computes line offset tables and returns the assigned 1-based source file ID, or 0 if already registered.
     */
    virtual size_t addSourceContent(const std::string &name, const std::string_view &content) = 0;

    /**
     * Creates an arena-allocated SourceReference descriptor for a byte range in the file designated by numeric ID.
     * Returns nullptr if source ID is invalid or offset exceeds buffer length.
     */
    virtual SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) = 0;

    /**
     * Creates an arena-allocated SourceReference descriptor using registered file name.
     * Returns nullptr if source file is not found in the registry.
     */
    virtual SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) = 0;

    /**
     * Performs binary search over precomputed line ranges to locate the line containing the given reference.
     * Returns a pointer to the matching SourceLineRange, or nullptr if not found.
     */
    virtual SourceLineRange *getReferenceLine(SourceReference *ref) const = 0;

    /**
     * Adds an include directory path to search during file resolution.
     */
    virtual void addIncludePath(const std::filesystem::path &path) = 0;

    /**
     * Resolves a source file path against relative directories, working directories, and search paths.
     * Returns the resolved canonical filesystem path.
     */
    virtual std::filesystem::path
    resolveSourcePath(const std::filesystem::path &sourceFile,
                      const std::optional<std::filesystem::path> &relativeTo = std::nullopt) const = 0;

    /**
     * Loads a file from disk into the source registry, precomputing line boundaries and assigning a unique source ID.
     * Returns the assigned source ID on success, or std::nullopt on I/O error.
     */
    virtual std::optional<size_t> loadFile(const std::filesystem::path &filePath,
                                           const std::optional<std::filesystem::path> &relativeTo = std::nullopt) = 0;

    /**
     * Returns a zero-copy string view of the complete line of source text enclosing the specified reference.
     */
    virtual std::string_view getRawLineContent(SourceReference *ref) const = 0;

    /**
     * Returns a zero-copy string view of the exact token/span referenced by the specified SourceReference.
     */
    virtual std::string_view getReferenceContent(SourceReference *ref) const = 0;

    /**
     * Returns the complete file content string view for the specified numeric source ID.
     */
    virtual std::string_view getSourceContent(size_t id) const = 0;

    /**
     * Returns the display name or path of the source file corresponding to the specified ID.
     */
    virtual std::string_view getSourceName(size_t id) const = 0;
};

#endif // EZCORE_GENERIC_SOURCE_MANAGER_H