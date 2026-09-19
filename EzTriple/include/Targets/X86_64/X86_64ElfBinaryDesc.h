#ifndef EZTRIPLE_X86_64_ELF_BINARY_DESC_H
#define EZTRIPLE_X86_64_ELF_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "CodeSection.h"
#include <memory_resource>

namespace EzTriple
{

/**
 * System V x86-64 (ELF) binary descriptor. Optionally configured for position-independent code.
 */
class X86_64ElfBinaryDesc : public TargetBinaryDesc
{
  public:
    /**
     * Creates the descriptor.
     * @param alloc Memory resource backing the section table.
     * @param isPic Enables position-independent code generation when true.
     */
    explicit X86_64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic = false);
    ~X86_64ElfBinaryDesc() override = default;

    /// ELF x86-64 targets are little endian.
    bool isLittleEndian() const override { return true; }

    /// Reflects the PIC flag supplied at construction time.
    bool isPositionIndependent() const override { return m_isPic; }

    /// Identifier used to register and look up this descriptor.
    const char *getName() const override { return "x86_64-elf"; }

    /// Returns the section backing the requested section type, creating it on demand.
    CodeSection *getSection(SectionType type) override;

    /// Uses the small code model, assuming addresses fit in a 32-bit immediate.
    TargetCodeModel getCodeModel() const override { return TargetCodeModel::Small; }

    /// Reports the ELF object container format.
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::ELF; }

    /// Functions are aligned to 16 bytes.
    size_t getFunctionAlignment() const override { return 16; }

    /// Loop headers are aligned to 16 bytes.
    size_t getLoopAlignment() const override { return 16; }

    /// Creates the concrete ELF sections and wires them into m_sections.
    void initialize() override;

    /// Returns the section lookup table populated by initialize().
    const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const override { return m_sections; }

  private:
    std::pmr::memory_resource *m_alloc;                             ///< Allocator backing the section table.
    bool m_isPic{ false };                                          ///< Whether position-independent code is requested.
    std::pmr::unordered_map<SectionType, CodeSection *> m_sections; ///< Section type to CodeSection mapping.
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_ELF_BINARY_DESC_H
