#include "Targets/X86_64/X86_64RelocationResolver.h"
#include "X86_64/Encoding/X86_64InstructionEncoder.h"

namespace EzTriple
{

namespace
{

/**
 * Locates a near branch/call's displacement field and the address of the following instruction
 * using the encoder's opcode knowledge, so the bytes-level layout lives in one place.
 */
bool resolveBranchField(std::span<const uint8_t> text, uint64_t relocOffset, uint64_t &dispOffset, uint64_t &nextRip)
{
    size_t fieldOffset = 0;
    size_t instrLength = 0;
    if (!EzCodeEmitter::X86_64::InstructionEncoder::classifyNearBranch(
                text, static_cast<size_t>(relocOffset), fieldOffset, instrLength))
    {
        return false;
    }

    dispOffset = fieldOffset;
    nextRip = relocOffset + instrLength;
    return true;
}

} // namespace

/**
 * Patches near branch/call displacements (BranchRel32) by inspecting the opcode to find the
 * displacement field and the address of the next instruction, or patches a bare RIP-relative
 * 32-bit field (PCRel32).
 */
bool X86_64RelocationResolver::patch(std::span<uint8_t> text,
                                     const CodeRelocation &reloc,
                                     uint64_t targetOffset,
                                     TargetCodeRelocationType type)
{
    const uint64_t relocOffset = reloc.m_address;
    if (relocOffset >= text.size())
    {
        return false;
    }

    if (type == TargetCodeRelocationType::BranchRel32)
    {
        uint64_t dispOffset = 0;
        uint64_t nextRip = 0;
        if (!resolveBranchField(text, relocOffset, dispOffset, nextRip))
        {
            return false;
        }

        // x86 relative branches are encoded as target - address_of_next_instruction.
        const int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextRip));
        return EzCodeEmitter::X86_64::InstructionEncoder::writeDisp32(text, dispOffset, disp);
    }

    if (type == TargetCodeRelocationType::PCRel32)
    {
        const uint64_t fieldOffset = reloc.m_address;

        // A RIP-relative field is measured from the end of the 4-byte field itself.
        const int32_t disp =
                static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(fieldOffset + 4));
        return EzCodeEmitter::X86_64::InstructionEncoder::writeDisp32(text, fieldOffset, disp);
    }

    return false;
}

/**
 * Resolves the fixup field offset using the same opcode knowledge as patch(), so object-file
 * relocations point at the displacement bytes rather than the start of the instruction.
 */
uint64_t X86_64RelocationResolver::getRelocationFieldOffset(std::span<const uint8_t> text,
                                                            const CodeRelocation &reloc,
                                                            TargetCodeRelocationType type) const
{
    if (type == TargetCodeRelocationType::BranchRel32)
    {
        uint64_t dispOffset = reloc.m_address;
        uint64_t nextRip = 0;
        if (resolveBranchField(text, reloc.m_address, dispOffset, nextRip))
        {
            return dispOffset;
        }
    }
    return reloc.m_address;
}

} // namespace EzTriple
