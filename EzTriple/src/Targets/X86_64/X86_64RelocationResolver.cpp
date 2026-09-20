#include "Targets/X86_64/X86_64RelocationResolver.h"

namespace EzTriple
{

namespace
{

/// Stores a 32-bit value in little-endian order at the given byte offset.
void writeLittleEndian32(std::span<uint8_t> text, uint64_t offset, uint32_t value)
{
    text[offset + 0] = static_cast<uint8_t>(value & 0xFF);
    text[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    text[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    text[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

/**
 * Sniffs the opcode at relocOffset to locate a near branch/call's displacement field and the
 * address of the following instruction. Returns false for unrecognized opcodes.
 */
bool resolveBranchField(std::span<const uint8_t> text, uint64_t relocOffset, uint64_t &dispOffset, uint64_t &nextRip)
{
    if (relocOffset >= text.size())
    {
        return false;
    }

    const uint8_t op0 = text[relocOffset];
    if (op0 == 0xE9 || op0 == 0xE8) // JMP rel32 / CALL rel32
    {
        dispOffset = relocOffset + 1;
        nextRip = relocOffset + 5;
        return true;
    }
    if (op0 == 0x0F && relocOffset + 1 < text.size() && (text[relocOffset + 1] & 0xF0) == 0x80) // Jcc rel32
    {
        dispOffset = relocOffset + 2;
        nextRip = relocOffset + 6;
        return true;
    }
    return false;
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

        if (dispOffset + 4 > text.size())
        {
            return false;
        }

        // x86 relative branches are encoded as target - address_of_next_instruction.
        const int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextRip));
        writeLittleEndian32(text, dispOffset, static_cast<uint32_t>(disp));
        return true;
    }

    if (type == TargetCodeRelocationType::PCRel32)
    {
        const uint64_t fieldOffset = reloc.m_address;
        if (fieldOffset + 4 > text.size())
        {
            return false;
        }

        // A RIP-relative field is measured from the end of the 4-byte field itself.
        const int32_t disp =
                static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(fieldOffset + 4));
        writeLittleEndian32(text, fieldOffset, static_cast<uint32_t>(disp));
        return true;
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
