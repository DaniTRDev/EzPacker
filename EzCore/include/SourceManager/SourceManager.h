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

    size_t m_sourceFileId;
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
     * Adds a new source file using given content and name.
     * @param name
     * @param content
     * @return bool
     */
    bool addSourceContent(const std::string &name, const std::string &content);

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
     * Returns the source name of the given source file id.
     * @param id
     * @return const std::string &
     */
    std::string_view getSourceName(size_t id) const;

  private:
    // full file path, file content divided in lines.
    std::map<size_t, std::vector<LineSourceRange>> m_sourceLines;
    std::map<size_t, std::string> m_sources;

    // id, name
    std::map<size_t, std::string> m_sourcesNames;
};

#endif // EZPACKER_SOURCEMANAGER_H
