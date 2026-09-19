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
};

#endif // EZTRIPLE_TARGET_RELOCATION_RESOLVER_H
