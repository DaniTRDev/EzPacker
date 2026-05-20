#ifndef EZPACKER_CODEBUFFER_H
#define EZPACKER_CODEBUFFER_H

#include "EzTargetEmitterCommon.h"
#include "CodeSection.h"

class CodeBuffer
{
  public:
    /**
     * Creates a section with the given parameters.
     * @return
     */
    CodeSection *createSection(CodeSectionFlags flags, ConstantArray<uint8_t> *data, const std::string_view &name);

    /**
     * Returns the code section whose name matches the given one. Returns nullptr if no match is found.
     * @param name
     * @return
     */
    CodeSection *getSection(const std::string_view &name);

    /**
     * Returns the relocation pool.
     * @return
     */
    const TypedPool &getRelocationPool() const;

    /**
     * Returns the pool used to store names of sections.
     * @return
     */
    const TypedPool &getSectionNamePool() const;

    /**
     * Returns the object that's used as an arena allocator by this buffer to save sections.
     * @return
     */
    const TypedPool &getSectionsPool() const;

    /**
     * Returns the list of sections used by this code buffer.
     * @return
     */
    TypedPoolLinkedList<CodeSection> *getSectionList() const;

    /**
     * Returns the section map.
     * @return
     */
    const std::map<std::string_view, CodeSection *> &getSectionMap() const;

  private:
    TypedPool m_relocationPool;
    TypedPool m_sectionsPool;
    TypedArrayPool<char> m_sectionNamePool;
    TypedPoolLinkedList<CodeSection> *m_sections;
    std::map<std::string_view, CodeSection *> m_sectionMap; // To optimize searches.
};

#endif // EZPACKER_CODEBUFFER_H
