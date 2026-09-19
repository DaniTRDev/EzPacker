#ifndef EZTRIPLE_X86_64_COFF_BINARY_DESC_H
#define EZTRIPLE_X86_64_COFF_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "CodeSection.h"
#include <memory_resource>

namespace EzTriple
{

/**
 * Windows x64 (COFF) binary descriptor. Code is non-PIC and uses the small code model.
 */
class X86_64CoffBinaryDesc : public TargetBinaryDesc
{
  public:
    /**
     * Creates the descriptor, allocating its section table from the provided memory resource.
     */
    explicit X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc);
    ~X86_64CoffBinaryDesc() override = default;

    /// Windows x64 targets are little endian.
    bool isLittleEndian() const override { return true; }

    /// Windows x64 default code model is non-position-independent.
    bool isPositionIndependent() const override { return false; }

    /// Identifier used to register and look up this descriptor.
    const char *getName() const override { return "x86_64-coff"; }

    /// Returns the section backing the requested section type, creating it on demand.
    CodeSection *getSection(SectionType type) override;

    /// Uses the small code model, assuming addresses fit in a 32-bit immediate.
    TargetCodeModel getCodeModel() const override { return TargetCodeModel::Small; }

    /// Reports the COFF object container format.
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::COFF; }

    /// Functions are aligned to 16 bytes on Windows x64.
    size_t getFunctionAlignment() const override { return 16; }

    /// Loop headers are aligned to 16 bytes.
    size_t getLoopAlignment() const override { return 16; }

    /// Creates the concrete COFF sections and wires them into m_sections.
    void initialize() override;

    /// Returns the section lookup table populated by initialize().
    const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const override { return m_sections; }

  private:
    std::pmr::memory_resource *m_alloc;                             ///< Allocator backing the section table.
    std::pmr::unordered_map<SectionType, CodeSection *> m_sections; ///< Section type to CodeSection mapping.
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_COFF_BINARY_DESC_H
