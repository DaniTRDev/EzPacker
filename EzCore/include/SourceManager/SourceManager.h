#ifndef EZCORE_SOURCE_MANAGER_H
#define EZCORE_SOURCE_MANAGER_H

#include "EzCoreCommon.h"

/**
 * This struct represents a reference to the source code.
 */
struct SourceReference
{
    bool m_valid{ false };
    size_t m_col{ 0 };
    size_t m_length{ 0 };
    size_t m_line{ 0 };

    size_t m_sourceFileId{ 0 };
};

struct LineSourceRange
{
    size_t m_start;  // Byte offset in the file where line starts
    size_t m_length; // Length of the line (excluding newline)
};

struct SourceFileEntry
{
    std::string name;
    std::string content;
    std::vector<LineSourceRange> lines;
};

/**
 * This class is responsible of debug symbols. It keeps references to the original content of
 * source files and translates references to content from the file.
 */
class SourceManager
{
  public:
    /**
     * Constructs a SourceManager with the given working path.
     */
    SourceManager(const std::filesystem::path &workingPath);

    /**
     * Checks if a source with the given name already exists in the manager.
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const;

    /**
     * Creates a source reference that can be used to show source content.
     */
    SourceReference createReference(size_t col, size_t length, size_t line, size_t sourceId);

    /**
     * Creates a source reference that can be used to show source content.
     */
    SourceReference createReference(size_t col, size_t length, size_t line, const std::string &sourceFile);

    /**
     * Adds a new source file using given content and name. Returns 0 if the source already existed.
     */
    size_t addSourceContent(const std::string &name, const std::string &content);

    /**
     * Resolves the given source file path to an absolute path based on the working directory.
     */
    std::filesystem::path resolveSourcePath(const std::filesystem::path &sourceFile) const;

    /**
     * Returns the raw line of where this reference was created. Returns an empty string if ref was not found.
     */
    std::string getRawLineContent(const SourceReference &ref) const;

    /**
     * Returns the line content of the given reference. Returns the content of the reference or an empty string if the
     * reference was not found.
     */
    std::string getReferenceContent(const SourceReference &ref) const;

    /**
     * Returns the source content for the given ID. If the source file was not foud, an empty string is returned.
     */
    std::string getSourceContent(size_t id) const;

    /**
     * Returns the source name of the given source file id.
     */
    std::string getSourceName(size_t id) const;

  private:
    std::filesystem::path m_workingPath;
    std::unordered_map<std::string, size_t> m_pathToIdMap;
    std::vector<SourceFileEntry> m_sourceFiles;
};

#endif // EZCORE_SOURCE_MANAGER_H
