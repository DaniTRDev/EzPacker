#ifndef EZPACKER_SOURCEMANAGER_H
#define EZPACKER_SOURCEMANAGER_H

#include "EzCoreCommon.h"

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

struct LineSourceRange
{
    size_t m_end;   // End of the line within the code buffer (ALWAYS \n character).
    size_t m_start; // Start of the line within the code buffer.
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
     * Creates a source reference that can be used to show source content.
     * @param col
     * @param length
     * @param line
     * @param sourceFile
     * @return std::shared_ptr<SourceReference>
     */
    std::shared_ptr<SourceReference>
    createReference(size_t col, size_t length, size_t line, const std::string &sourceFile);

    /**
     * Merges the given references into a single reference. This function SUPPOSES that references are one after
     * another. Can't merge 2 references that are not close.
     * @param refs
     * @return std::shared_ptr<SourceReference>
     */
    std::shared_ptr<SourceReference> mergeReferences(const std::vector<std::shared_ptr<SourceReference>> &refs);

    /**
     * Returns the line content of the given reference. This function assumes ref is DEFINED.
     * @param ref
     * @return std::string
     */
    std::string getReferenceContent(const std::shared_ptr<SourceReference> &ref);

  private:
    // full file path, file content, divided in lines.
    std::map<std::string, std::vector<LineSourceRange>> m_sourceLines;
    std::map<std::string, std::string> m_sources;
};

/**
 * Simple class that allows creating a single reference out of multiple references (merging) easier and cleaner.
 */
class MultiSourceReferenceCreator
{
  public:
  
    /**
     * Attaches this creator to a SourceManager. If given source manager is null, an exception is thrown.
     * @param sourceManager
     */
    void attach(std::shared_ptr<SourceManager> sourceManager);
  
    /**
     * Pushes a reference to the creator. If given reference is null, an exception is thrown.
     * @param reference
     */
    void push(std::shared_ptr<SourceReference> reference);
    
    /**
     * Merges all the given references into a single returned reference. Throws an exception if no references were
     * given, source manager was not set or resulting reference is invalid.
     */
    std::shared_ptr<SourceReference> merge();
    
  private:
    std::shared_ptr<SourceManager> m_sourceManager;
    std::vector<std::shared_ptr<SourceReference>> m_references;
};

#endif // EZPACKER_SOURCEMANAGER_H
