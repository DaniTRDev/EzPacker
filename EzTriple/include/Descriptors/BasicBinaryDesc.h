#ifndef EZTRIPLE_BASIC_BINARY_DESC_H
#define EZTRIPLE_BASIC_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetBinaryDesc.h"

#include <memory_resource>

namespace EzTriple
{

/**
 * Shared base for concrete target binary descriptors backed by a PMR section table.
 *
 * Supplies the common little-endian, 16-byte function alignment default and the section lookup
 * that the ELF and COFF descriptors would otherwise duplicate. Derived descriptors only provide
 * their name, object format, PIC default and section factory.
 */
class BasicBinaryDesc : public TargetBinaryDesc
{
  public:
    /**
     * Creates the descriptor with an allocator-owned section table.
     * @param alloc Memory resource backing the section table.
     * @param isPic Whether position-independent code is requested.
     */
    explicit BasicBinaryDesc(std::pmr::memory_resource *alloc, bool isPic = false);
    ~BasicBinaryDesc() override = default;

    /// All descriptors built on this base are little endian.
    bool isLittleEndian() const override { return true; }

    /// Reflects the PIC flag supplied at construction time.
    bool isPositionIndependent() const override { return m_isPic; }

    /// Functions are aligned to 16 bytes.
    size_t getFunctionAlignment() const override { return 16; }

    /// Returns the section backing the requested section type, or nullptr when absent.
    CodeSection *getSection(SectionType type) override;

    /// Returns the section lookup table populated by initialize().
    const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const override { return m_sections; }

  protected:
    std::pmr::memory_resource *m_alloc;                             ///< Allocator backing the section table.
    bool m_isPic{ false };                                          ///< Whether position-independent code is requested.
    std::pmr::unordered_map<SectionType, CodeSection *> m_sections; ///< Section type to CodeSection mapping.
};

} // namespace EzTriple

#endif // EZTRIPLE_BASIC_BINARY_DESC_H
