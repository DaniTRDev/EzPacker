#ifndef EZPACKER_SOURCEMANAGER_H
#define EZPACKER_SOURCEMANAGER_H

#include "EzFrontendCommon.h"

/**
 * This struct represents a reference to the source code.
 */
struct SourceReference
{
    size_t m_col;
    size_t m_length;
    size_t m_line;

    std::string m_sourceFile; // Includes path.
};

/**
 * This class is responsible of debug symbols. It keeps references to the original content of
 * source files and translates references to content from the file.
 */
class SourceManager
{
  public:
    /**
     * Crates the object with default values.
     */
    SourceManager();

    /**
     * Destroys the object and free resources.
     */
    ~SourceManager();

    /**
     * Adds a new source file using given content and name.
     * @param name
     * @param content
     * @return bool
     */
    bool addSourceContent(const std::string &name, const std::string &content);

    /**
     * Adds a source file to the manager (its FULL path). Returns false if it was already added.
     * @param sourceFile
     * @param content
     * @return bool
     */
    bool addSourceFile(const std::string &sourceFile, std::vector<std::string> content);

    /**
     * Creates a source reference that can be used to show source content.
     * @param col
     * @param length
     * @param line
     * @param sourceFile
     * @return std::shared_ptr<SourceReference>
     */
    std::shared_ptr<SourceReference> createReference(size_t col, size_t length, size_t line,
                                                     const std::string &sourceFile);

    /**
     * Returns the line content of the given reference. This function assumes ref is DEFINED and VALID. Returns a view
     * of the line of the source file. Modification is FORBIDDEN.
     * @param ref
     * @return std::string_view
     */
    std::string_view getReferenceContent(const std::shared_ptr<SourceReference> &ref);

  private:
    // full file path, file content, divided in lines.
    std::unordered_map<std::string, std::vector<std::string>> m_source;
};

#endif // EZPACKER_SOURCEMANAGER_H
