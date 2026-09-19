#ifndef EZTRIPLE_X86_64_RELOCATION_RESOLVER_H
#define EZTRIPLE_X86_64_RELOCATION_RESOLVER_H

#include "Descriptors/TargetRelocationResolver.h"
#include "EzTripleCommon.h"

namespace EzTriple
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
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_RELOCATION_RESOLVER_H
