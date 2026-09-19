#include "Targets/X86_64/X86_64RelocationResolver.h"

namespace EzTriple
{

namespace
{

void writeLittleEndian32(std::span<uint8_t> text, uint64_t offset, uint32_t value)
{
    text[offset + 0] = static_cast<uint8_t>(value & 0xFF);
    text[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    text[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    text[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

} // namespace

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
        const uint8_t op0 = text[relocOffset];
        uint64_t dispOffset = 0;
        uint64_t nextRip = 0;

        if (op0 == 0xE9 || op0 == 0xE8) // JMP rel32 / CALL rel32
        {
            dispOffset = relocOffset + 1;
            nextRip = relocOffset + 5;
        }
        else if (op0 == 0x0F && relocOffset + 1 < text.size() && (text[relocOffset + 1] & 0xF0) == 0x80) // Jcc rel32
        {
            dispOffset = relocOffset + 2;
            nextRip = relocOffset + 6;
        }
        else
        {
            return false;
        }

        if (dispOffset + 4 > text.size())
        {
            return false;
        }

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

        const int32_t disp =
                static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(fieldOffset + 4));
        writeLittleEndian32(text, fieldOffset, static_cast<uint32_t>(disp));
        return true;
    }

    return false;
}

} // namespace EzTriple
