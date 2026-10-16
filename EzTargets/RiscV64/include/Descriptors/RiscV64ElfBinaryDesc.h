#ifndef EZTARGETS_RISCV64_ELF_BINARY_DESC_H
#define EZTARGETS_RISCV64_ELF_BINARY_DESC_H

#include "EzTargetsRiscV64Common.h"
#include "Descriptors/BasicBinaryDesc.h"

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64-bit (ELF) binary descriptor. Optionally configured for position-independent code.
 */
class RiscV64ElfBinaryDesc : public EzTriple::BasicBinaryDesc
{
  public:
    /**
     * Creates the descriptor.
     * @param alloc Memory resource backing the section table.
     * @param isPic Enables position-independent code generation when true.
     */
    explicit RiscV64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic = false);
    ~RiscV64ElfBinaryDesc() override = default;

    /// Identifier used to register and look up this descriptor.
    const char *getName() const override { return "riscv64-elf"; }

    /// Reports the ELF object container format.
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::ELF; }

    /// Creates the concrete ELF sections and wires them into m_sections.
    void initialize() override;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_ELF_BINARY_DESC_H
