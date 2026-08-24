#ifndef EZCORE_SOURCE_MANAGER_H
#define EZCORE_SOURCE_MANAGER_H

#include "EzCoreCommon.h"
#include "GenericSourceManager.h"
#include <filesystem>
#include <optional>

/**
 * This class is responsible for debug symbols and source tracking. It keeps references to
 * the original content of source files and translates references to content from the file.
 */
class SourceManager : public GenericSourceManager
{
  public:
    /**
     * Constructs a SourceManager with the given working path and memory resource.
     */
    SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc);

    /**
     * Checks if a source with the given name already exists in the manager.
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const override;

    /**
     * Adds a new source file using given content and name. Returns 0 if the source already existed.
     */
    size_t addSourceContent(const std::string &name, const std::string_view &content) override;

    /**
     * Creates a source reference using start offset, length, and source file ID.
     */
    SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) override;

    /**
     * Creates a source reference using start offset, length, and source file name.
     */
    SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) override;

    /**
     * Returns the source line range that contains the given reference.
     */
    SourceLineRange *getReferenceLine(SourceReference *ref) const override;

    /**
     * Adds an include directory to search for included or referenced files.
     */
    void addIncludePath(const std::filesystem::path &path) override;

    /**
     * Resolves the given source file path based on the working directory, relative base, and include paths.
     */
    std::filesystem::path
    resolveSourcePath(const std::filesystem::path &sourceFile,
                      const std::optional<std::filesystem::path> &relativeTo = std::nullopt) const override;

    /**
     * Loads a file from disk into the source manager, resolving its path against include paths
     * and the working directory. Returns the assigned source ID, or std::nullopt on failure.
     */
    std::optional<size_t> loadFile(const std::filesystem::path &filePath,
                                   const std::optional<std::filesystem::path> &relativeTo = std::nullopt) override;

    /**
     * Returns a direct pointer to the arena-backed source buffer, or nullptr if id is invalid.
     */
    const std::pmr::string *getSourceBuffer(size_t id) const;

    /**
     * Returns all registered include search paths.
     */
    const std::pmr::vector<std::filesystem::path> &getIncludePaths() const;

    /**
     * Returns the raw line of where this reference was created. Returns an empty string if ref was not found.
     *
     * This method returns a view to the internal content buffer.
     */
    std::string_view getRawLineContent(SourceReference *ref) const override;

    /**
     * Returns the line content of the given reference. Returns the content of the reference or an empty string if the
     * reference was not found.
     */
    std::string_view getReferenceContent(SourceReference *ref) const override;

    /**
     * Returns the source content for the given ID. If the source file was not found, an empty string is returned.
     */
    std::string_view getSourceContent(size_t id) const override;

    /**
     * Returns the source name of the given source file id.
     */
    std::string_view getSourceName(size_t id) const override;

  private:
    std::filesystem::path m_workingPath;
    std::pmr::memory_resource *m_alloc;
    std::pmr::vector<std::filesystem::path> m_includePaths;
    std::pmr::unordered_map<std::string_view, size_t> m_pathToIdMap;
    std::pmr::vector<SourceFileEntry *> m_sourceFiles;
};

#endif // EZCORE_SOURCE_MANAGER_H