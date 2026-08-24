#ifndef EZCORE_GENERIC_SOURCE_MANAGER_H
#define EZCORE_GENERIC_SOURCE_MANAGER_H

#include "EzCoreCommon.h"
#include <filesystem>
#include <optional>

/**
 * This struct represents a reference to the source code.
 */
struct SourceReference
{
    size_t m_beginOffset{ 0 };
    size_t m_endOffset{ 0 };
    size_t m_sourceFileId{ 0 };

    /**
     * Returns the length of the source reference in bytes.
     */
    size_t length() const { return m_endOffset - m_beginOffset; }
};

/**
 * Represents a discrete source line range, mapping byte offsets to a 1-based line number.
 */
struct SourceLineRange
{
    size_t m_beginOffset{ 0 };
    size_t m_endOffset{ 0 };
    size_t m_lineNumber{ 0 }; // 1-based line index

    /**
     * Returns the length of the line in bytes.
     */
    size_t length() const { return m_endOffset - m_beginOffset; }
};

/**
 * Cached source file descriptor containing raw file content, display name, and precomputed line bounds.
 */
struct SourceFileEntry
{
    std::pmr::string m_content;
    std::pmr::string m_name;
    std::pmr::vector<SourceLineRange> m_lines;
};

/**
 * Interface used to define plug and play source managers.
 */
class GenericSourceManager
{
  public:
    virtual ~GenericSourceManager() = default;

    /**
     * Checks if a source with the given name already exists in the manager.
     */
    virtual bool doesSourceNameExist(const std::string_view &sourceName) const = 0;

    /**
     * Adds a new source file using given content and name. Returns 0 if the source already existed.
     */
    virtual size_t addSourceContent(const std::string &name, const std::string_view &content) = 0;

    /**
     * Creates a source reference using start offset, length, and source file ID.
     */
    virtual SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) = 0;

    /**
     * Creates a source reference using start offset, length, and source file name.
     */
    virtual SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) = 0;

    /**
     * Returns the source line range that contains the given reference.
     */
    virtual SourceLineRange *getReferenceLine(SourceReference *ref) const = 0;

    /**
     * Adds an include directory to search for included or referenced files.
     */
    virtual void addIncludePath(const std::filesystem::path &path) = 0;

    /**
     * Resolves the given source file path based on the working directory, relative base, and include paths.
     */
    virtual std::filesystem::path
    resolveSourcePath(const std::filesystem::path &sourceFile,
                      const std::optional<std::filesystem::path> &relativeTo = std::nullopt) const = 0;

    /**
     * Loads a file from disk into the source manager, resolving its path against include paths
     * and the working directory. Returns the assigned source ID, or std::nullopt on failure.
     */
    virtual std::optional<size_t> loadFile(const std::filesystem::path &filePath,
                                           const std::optional<std::filesystem::path> &relativeTo = std::nullopt) = 0;

    /**
     * Returns the raw line of where this reference was created. Returns an empty string if ref was not found.
     *
     * This method returns a view to the internal content buffer.
     */
    virtual std::string_view getRawLineContent(SourceReference *ref) const = 0;

    /**
     * Returns the line content of the given reference. Returns the content of the reference or an empty string if the
     * reference was not found.
     */
    virtual std::string_view getReferenceContent(SourceReference *ref) const = 0;

    /**
     * Returns the source content for the given ID. If the source file was not found, an empty string is returned.
     */
    virtual std::string_view getSourceContent(size_t id) const = 0;

    /**
     * Returns the source name of the given source file id.
     */
    virtual std::string_view getSourceName(size_t id) const = 0;
};

#endif // EZCORE_GENERIC_SOURCE_MANAGER_H