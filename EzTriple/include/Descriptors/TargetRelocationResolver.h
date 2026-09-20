#ifndef EZTRIPLE_TARGET_RELOCATION_RESOLVER_H
#define EZTRIPLE_TARGET_RELOCATION_RESOLVER_H

#include "CodeEmitterContext.h"
#include "EzTripleCommon.h"

#include <cstdint>
#include <span>

/**
 * Target hook owning the "given a relocation and section bytes, patch the right field"
 * logic. Implementations live beside their target descriptor so that EmissionEngine can
 * stay target-agnostic and simply delegate relocation patching.
 */
class TargetRelocationResolver
{
  public:
    virtual ~TargetRelocationResolver() = default;

    /**
     * Patches the encoded relocation field in place.
     *
     * @param text         Mutable bytes of the section that owns the relocation.
     * @param reloc        Relocation record describing the field location.
     * @param targetOffset Absolute offset of the resolved target symbol within the section.
     * @param type         Relocation kind requested by the emitter.
     * @return true when the relocation was handled, false when unsupported.
     */
    virtual bool patch(std::span<uint8_t> text,
                       const CodeRelocation &reloc,
                       uint64_t targetOffset,
                       TargetCodeRelocationType type) = 0;

    /**
     * Returns the byte offset (within the section) of the relocation's fixup field.
     *
     * For types whose field location depends on the encoded opcode (e.g. near branches) the target
     * inspects the section bytes; other types fall back to reloc.m_address. This lets the compiler
     * describe object-file relocations without re-implementing opcode knowledge.
     *
     * @param text  Finalized section bytes that own the relocation.
     * @param reloc Relocation record describing the instruction/field location.
     * @param type  Relocation kind requested by the emitter.
     * @return Absolute offset of the field to patch.
     */
    virtual uint64_t getRelocationFieldOffset(std::span<const uint8_t> text,
                                              const CodeRelocation &reloc,
                                              TargetCodeRelocationType type) const
    {
        (void)text;
        (void)type;
        return reloc.m_address;
    }
};

#endif // EZTRIPLE_TARGET_RELOCATION_RESOLVER_H
