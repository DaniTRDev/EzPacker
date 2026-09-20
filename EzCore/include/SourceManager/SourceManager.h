#ifndef EZCORE_SOURCE_MANAGER_H
#define EZCORE_SOURCE_MANAGER_H

#include "EzCoreCommon.h"
#include "GenericSourceManager.h"
#include <filesystem>
#include <optional>

/**
 * Concrete source file registry managing arena-backed file buffers, include paths, and debug symbol spans.
 * Provides thread-compatible storage of SourceFileEntry instances, line-to-offset binary mapping,
 * and canonical include path resolution for the compiler frontend and AST/MIR diagnostics.
 */
class SourceManager : public GenericSourceManager
{
  public:
    /**
     * Constructs a SourceManager with a root working directory and a PMR memory resource for arena allocations.
     */
    SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc);

    /**
     * Destructor freeing all allocated SourceFileEntry objects through the memory resource.
     */
    ~SourceManager();

    // Non-copyable: entries and map keys are raw pointers into the arena, so a copy would
    // double-free and leave the copied map keys dangling. Ownership is always by pointer.
    SourceManager(const SourceManager &) = delete;
    SourceManager &operator=(const SourceManager &) = delete;

    /**
     * Checks if a source buffer with the given name or path exists in the path-to-ID lookup map.
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const override;

    /**
     * Adds an in-memory source file with the specified name and content string view.
     * Computes line bounds and registers the entry. Returns the new 1-based ID, or 0 if name already exists.
     */
    size_t addSourceContent(const std::string &name, const std::string_view &content) override;

    /**
     * Allocates and initializes a SourceReference for a byte interval within the file indicated by sourceId.
     * Returns nullptr if sourceId is invalid or startOffset exceeds file length.
     */
    SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) override;

    /**
     * Allocates and initializes a SourceReference using registered source file name.
     * Returns nullptr if source file is not found in the registry.
     */
    SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) override;

    /**
     * Finds the precomputed 1-based line interval enclosing the given SourceReference via binary search.
     * Returns nullptr if reference or source entry is invalid.
     */
    SourceLineRange *getReferenceLine(SourceReference *ref) const override;

    /**
     * Adds an include directory to the search list, converting existing paths to weakly canonical forms.
     */
    void addIncludePath(const std::filesystem::path &path) override;

    /**
     * Resolves a file path relative to an including file, the working directory, or registered include paths.
     * Returns the weakly canonical path.
     */
    std::filesystem::path
    resolveSourcePath(const std::filesystem::path &sourceFile,
                      const std::optional<std::filesystem::path> &relativeTo = std::nullopt) const override;

    /**
     * Reads a source file from disk into the PMR arena, creates line index entries, and assigns a source ID.
     * Avoids duplicate loads if canonical path is already registered. Returns assigned ID or std::nullopt on error.
     */
    std::optional<size_t> loadFile(const std::filesystem::path &filePath,
                                   const std::optional<std::filesystem::path> &relativeTo = std::nullopt) override;

    /**
     * Returns a zero-copy string view of the full line containing the given SourceReference.
     */
    std::string_view getRawLineContent(SourceReference *ref) const override;

    /**
     * Returns a zero-copy string view of the exact text span referenced by the given SourceReference.
     */
    std::string_view getReferenceContent(SourceReference *ref) const override;

    /**
     * Returns the full content string view for the source file with the given numeric ID.
     */
    std::string_view getSourceContent(size_t id) const override;

    /**
     * Returns the registered name or path for the source file with the given numeric ID.
     */
    std::string_view getSourceName(size_t id) const override;

  private:
    std::filesystem::path m_workingPath; // Base directory used to resolve relative source paths.
    std::pmr::memory_resource *m_alloc;  // Arena that owns file entries and their buffer/line storage.
    std::pmr::vector<std::filesystem::path> m_includePaths;          // Search directories for include resolution.
    std::pmr::unordered_map<std::string_view, size_t> m_pathToIdMap; // Canonical path/name -> 1-based source ID.
    std::pmr::vector<SourceFileEntry *> m_sourceFiles; // Indexed by (ID - 1); owns each loaded file entry.
};

#endif // EZCORE_SOURCE_MANAGER_H