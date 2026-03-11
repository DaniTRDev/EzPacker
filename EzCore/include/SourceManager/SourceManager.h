#ifndef EZPACKER_SOURCEMANAGER_H
#define EZPACKER_SOURCEMANAGER_H

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

/**
 * This class is responsible of debug symbols. It keeps references to the original content of
 * source files and translates references to content from the file.
 */
class SourceManager
{
  public:
    /**
     * Constructs a SourceManager with the given working path. The working path is used to resolve relative paths for
     * source files.
     * @param workingPath
     */
    SourceManager(const std::filesystem::path &workingPath);

    /**
     * Checks if a source with the given name already exists in the manager.
     * @param sourceName
     * @return bool
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const;

    /**
     * Creates a source reference that can be used to show source content.
     * @param col
     * @param length
     * @param line
     * @param sourceId
     * @return SourceReference
     */
    SourceReference createReference(size_t col, size_t length, size_t line, size_t sourceId);

    /**
     * Creates a source reference that can be used to show source content.
     * @param col
     * @param length
     * @param line
     * @param sourceFile
     * @return SourceReference
     */
    SourceReference createReference(size_t col, size_t length, size_t line, const std::string &sourceFile);

    /**
     * Adds a new source file using given content and name. Returns the ID of the source file, which is a hash of the
     * name. If a source with the same name already exists, it returns 0, indicating failure to add the source.
     * @param name
     * @param content
     * @return size_t
     */
    size_t addSourceContent(const std::string &name, const std::string &content);

    /**
     * Returns the working path of the source manager.
     * @return const std::filesystem::path &
     */
    const std::filesystem::path &getWorkingPath() const;

    /**
     * Resolves the given source file path to an absolute path based on the working directory. If the source file is
     * already an absolute path, it returns it as is. If the source file is a relative path, it combines it with the
     * working directory to produce an absolute path.
     * @param sourceFile
     * @return std::filesystem::path
     */
    std::filesystem::path resolveSourcePath(const std::filesystem::path &sourceFile) const;

    /**
     * Returns the raw line of where this reference was created. Returns true if no reference is given or if it is not
     * from any known sources.
     * @param ref
     * @return std::string
     */
    std::string getRawLineContent(const SourceReference &ref);

    /**
     * Returns the line content of the given reference. This function assumes ref is DEFINED.
     * @param ref
     * @return std::string
     */
    std::string getReferenceContent(const SourceReference &ref);

    /**
     * Returns the source content for the given ID. This is the full content of the source file. If ID is not found, it
     * returns an empty string.
     * @param id
     * @return std::string_view
     */
    std::string_view getSourceContent(size_t id) const;

    /**
     * Returns the source name of the given source file id.
     * @param id
     * @return const std::string &
     */
    std::string_view getSourceName(size_t id) const;

  private:
  private:
    std::filesystem::path m_workingPath;
    // full file path, file content divided in lines.
    std::map<size_t, std::vector<LineSourceRange>> m_sourceLines;
    std::map<size_t, std::string> m_sources;

    // id, name
    std::map<size_t, std::string> m_sourcesNames;
};

#endif // EZPACKER_SOURCEMANAGER_H
