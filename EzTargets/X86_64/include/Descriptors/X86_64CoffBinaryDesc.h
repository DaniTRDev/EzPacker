#ifndef EZTARGETS_X86_64_COFF_BINARY_DESC_H
#define EZTARGETS_X86_64_COFF_BINARY_DESC_H

#include "EzTargetsX86_64Common.h"
#include "Descriptors/BasicBinaryDesc.h"

namespace EzTargets::X86_64
{

/**
 * Windows x64 (COFF) binary descriptor. Code is non-PIC and uses the small code model.
 */
class X86_64CoffBinaryDesc : public EzTriple::BasicBinaryDesc
{
  public:
    /**
     * Creates the descriptor, allocating its section table from the provided memory resource.
     */
    explicit X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc);
    ~X86_64CoffBinaryDesc() override = default;

    /// Identifier used to register and look up this descriptor.
    const char *getName() const override { return "x86_64-coff"; }

    /// Reports the COFF object container format.
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::COFF; }

    /// Creates the concrete COFF sections and wires them into m_sections.
    void initialize() override;
};

} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_COFF_BINARY_DESC_H
