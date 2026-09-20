#ifndef EZTRIPLE_X86_64_ELF_BINARY_DESC_H
#define EZTRIPLE_X86_64_ELF_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/BasicBinaryDesc.h"

namespace EzTriple
{

/**
 * System V x86-64 (ELF) binary descriptor. Optionally configured for position-independent code.
 */
class X86_64ElfBinaryDesc : public BasicBinaryDesc
{
  public:
    /**
     * Creates the descriptor.
     * @param alloc Memory resource backing the section table.
     * @param isPic Enables position-independent code generation when true.
     */
    explicit X86_64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic = false);
    ~X86_64ElfBinaryDesc() override = default;

    /// Identifier used to register and look up this descriptor.
    const char *getName() const override { return "x86_64-elf"; }

    /// Reports the ELF object container format.
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::ELF; }

    /// Creates the concrete ELF sections and wires them into m_sections.
    void initialize() override;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_ELF_BINARY_DESC_H
