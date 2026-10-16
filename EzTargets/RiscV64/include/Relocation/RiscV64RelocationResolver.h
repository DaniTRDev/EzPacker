#ifndef EZTARGETS_RISCV64_RELOCATION_RESOLVER_H
#define EZTARGETS_RISCV64_RELOCATION_RESOLVER_H

#include "Descriptors/TargetRelocationResolver.h"
#include "EzTargetsRiscV64Common.h"

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64 relocation resolver and patcher.
 * Handles B-type branches, J-type jumps (JAL), and PC-relative fixups.
 */
class RiscV64RelocationResolver : public TargetRelocationResolver
{
  public:
    bool patch(std::span<uint8_t> text,
               const CodeRelocation &reloc,
               uint64_t targetOffset,
               TargetCodeRelocationType type) override;

    uint64_t getRelocationFieldOffset(std::span<const uint8_t> text,
                                      const CodeRelocation &reloc,
                                      TargetCodeRelocationType type) const override;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_RELOCATION_RESOLVER_H
