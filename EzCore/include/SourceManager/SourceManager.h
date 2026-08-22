#ifndef EZCORE_SOURCE_MANAGER_H
#define EZCORE_SOURCE_MANAGER_H

#include "EzCoreCommon.h"
#include "GenericSourceManager.h"

/**
 * This class is responsible of debug symbols. It keeps references to the original content of
 * source files and translates references to content from the file.
 */
class SourceManager : public GenericSourceManager
{
  public:
    /**
     * Constructs a SourceManager with the given working path.
     */
    SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc);

    /**
     * Checks if a source with the given name already exists in the manager.
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const override;

    /**
     * Returns the source line that contains the given reference.
     */
    SourceLineRange *getReferenceLine(SourceReference *ref) const override;

    /**
     * Creates a source reference that can be used to show source content.
     */
    SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) override;

    /**
     * Creates a source reference that can be used to show source content.
     */
    SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) override;

    /**
     * Adds a new source file using given content and name. Returns 0 if the source already existed.
     */
    size_t addSourceContent(const std::string &name, const std::string_view &content) override;

    /**
     * Resolves the given source file path to an absolute path based on the working directory.
     */
    std::filesystem::path resolveSourcePath(const std::filesystem::path &sourceFile) const override;

    /**
     * Returns the raw line of where this reference was created. Returns an empty string if ref was not found.
     */
    std::string_view getRawLineContent(SourceReference *ref) const override;

    /**
     * Returns the line content of the given reference. Returns the content of the reference or an empty string if the
     * reference was not found.
     */
    std::string_view getReferenceContent(SourceReference *ref) const override;

    /**
     * Returns the source content for the given ID. If the source file was not foud, an empty string is returned.
     */
    std::string_view getSourceContent(size_t id) const override;

    /**
     * Returns the source name of the given source file id.
     */
    std::string_view getSourceName(size_t id) const override;

  private:
    std::filesystem::path m_workingPath;
    std::pmr::memory_resource *m_alloc;
    std::pmr::unordered_map<std::string_view, size_t> m_pathToIdMap;
    std::pmr::vector<SourceFileEntry *> m_sourceFiles;
};

#endif // EZCORE_SOURCE_MANAGER_H
