#ifndef EZTARGETS_X86_64_RELOCATION_RESOLVER_H
#define EZTARGETS_X86_64_RELOCATION_RESOLVER_H

#include "Descriptors/TargetRelocationResolver.h"
#include "EzTargetsX86_64Common.h"

namespace EzTargets::X86_64
{

/**
 * x86-64 relocation patcher. Handles the near branch/call opcodes emitted by the x86-64
 * emitter (0xE9 JMP, 0xE8 CALL, 0x0F 0x8x Jcc) and direct RIP-relative 32-bit fields.
 */
class X86_64RelocationResolver : public TargetRelocationResolver
{
  public:
    /**
     * Writes the PC-relative or absolute field described by reloc into the section bytes.
     * @return True when the relocation kind is supported and patched.
     */
    bool patch(std::span<uint8_t> text,
               const CodeRelocation &reloc,
               uint64_t targetOffset,
               TargetCodeRelocationType type) override;

    /**
     * Returns the offset of the fixup field for a near branch/call (opcode dependent) or the
     * relocation address itself for direct RIP-relative fields.
     */
    uint64_t getRelocationFieldOffset(std::span<const uint8_t> text,
                                      const CodeRelocation &reloc,
                                      TargetCodeRelocationType type) const override;
};

} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_RELOCATION_RESOLVER_H
