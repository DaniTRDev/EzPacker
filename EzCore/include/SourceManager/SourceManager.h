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
     * @brief Constructs a SourceManager with the given working path.
     * @param workingPath The working path to resolve relative paths for source files.
     */
    SourceManager(const std::filesystem::path &workingPath);

    /**
     * @brief Checks if a source with the given name already exists in the manager.
     * @param sourceName The name of the source to check.
     * @returns True if the source exists, false otherwise.
     */
    bool doesSourceNameExist(const std::string_view &sourceName) const;

    /**
     * @brief Creates a source reference that can be used to show source content.
     * @param col The column index of the reference.
     * @param length The length of the reference.
     * @param line The line index of the reference.
     * @param sourceId The ID of the source file.
     * @returns The created SourceReference.
     */
    SourceReference createReference(size_t col, size_t length, size_t line, size_t sourceId);

    /**
     * @brief Creates a source reference that can be used to show source content.
     * @param col The column index of the reference.
     * @param length The length of the reference.
     * @param line The line index of the reference.
     * @param sourceFile The path of the source file.
     * @returns The created SourceReference.
     */
    SourceReference createReference(size_t col, size_t length, size_t line, const std::string &sourceFile);

    /**
     * @brief Adds a new source file using given content and name.
     * @param name The name of the source file.
     * @param content The content of the source file.
     * @returns The ID of the source file, or 0 if a source with the same name already exists.
     */
    size_t addSourceContent(const std::string &name, const std::string &content);

    /**
     * @brief Returns the working path of the source manager.
     * @returns The working path.
     */
    const std::filesystem::path &getWorkingPath() const;

    /**
     * @brief Resolves the given source file path to an absolute path based on the working directory.
     * @param sourceFile The path to resolve.
     * @returns The resolved absolute path.
     */
    std::filesystem::path resolveSourcePath(const std::filesystem::path &sourceFile) const;

    /**
     * @brief Returns the raw line of where this reference was created.
     * @param ref The source reference.
     * @returns The raw line content, or an empty string if not found.
     */
    std::string getRawLineContent(const SourceReference &ref);

    /**
     * @brief Returns the line content of the given reference.
     * @param ref The source reference.
     * @returns The line content.
     */
    std::string getReferenceContent(const SourceReference &ref);

    /**
     * @brief Returns the source content for the given ID.
     * @param id The source file ID.
     * @returns The full content of the source file, or an empty string if not found.
     */
    std::string_view getSourceContent(size_t id) const;

    /**
     * @brief Returns the source name of the given source file id.
     * @param id The source file ID.
     * @returns The source name.
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
