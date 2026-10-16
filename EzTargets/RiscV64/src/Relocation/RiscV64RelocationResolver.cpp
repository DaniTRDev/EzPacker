#include "RiscV64RelocationResolver.h"
#include "Encoding/RiscV64InstructionEncoder.h"

namespace EzTargets::RiscV64
{

bool RiscV64RelocationResolver::patch(std::span<uint8_t> text,
                                     const CodeRelocation &reloc,
                                     uint64_t targetOffset,
                                     TargetCodeRelocationType type)
{
    const uint64_t relocOffset = reloc.m_address;
    if (relocOffset + 4 > text.size())
    {
        return false;
    }

    if (type == TargetCodeRelocationType::BranchRel32)
    {
        bool isJal = false;
        bool isBranch = false;
        uint8_t funct3 = 0, rs1 = 0, rs2 = 0, rd = 0;
        if (!InstructionEncoder::classifyBranch(text, static_cast<size_t>(relocOffset), isJal, isBranch, funct3, rs1, rs2, rd))
        {
            return false;
        }

        const int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(relocOffset));
        if (isJal)
        {
            return InstructionEncoder::patchJalDisp(text, static_cast<size_t>(relocOffset), disp);
        }
        if (isBranch)
        {
            return InstructionEncoder::patchBranchDisp(text, static_cast<size_t>(relocOffset), disp);
        }
        return false;
    }

    if (type == TargetCodeRelocationType::PCRel32)
    {
        const int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(relocOffset));

        // Check if instruction is AUIPC (opcode 0x17)
        uint32_t word = static_cast<uint32_t>(text[relocOffset]) |
                        (static_cast<uint32_t>(text[relocOffset + 1]) << 8) |
                        (static_cast<uint32_t>(text[relocOffset + 2]) << 16) |
                        (static_cast<uint32_t>(text[relocOffset + 3]) << 24);
        if ((word & 0x7F) == 0x17)
        {
            // AUIPC high 20 bits with rounding for 12-bit signed lower part
            int32_t hi20 = (disp + 0x800) >> 12;
            word = (word & 0xFFF) | ((static_cast<uint32_t>(hi20) & 0xFFFFF) << 12);
            text[relocOffset] = static_cast<uint8_t>(word & 0xFF);
            text[relocOffset + 1] = static_cast<uint8_t>((word >> 8) & 0xFF);
            text[relocOffset + 2] = static_cast<uint8_t>((word >> 16) & 0xFF);
            text[relocOffset + 3] = static_cast<uint8_t>((word >> 24) & 0xFF);
            return true;
        }

        return InstructionEncoder::writeDisp32(text, static_cast<size_t>(relocOffset), disp);
    }

    return false;
}

uint64_t RiscV64RelocationResolver::getRelocationFieldOffset(std::span<const uint8_t> text,
                                                            const CodeRelocation &reloc,
                                                            TargetCodeRelocationType type) const
{
    (void)text;
    (void)type;
    return reloc.m_address;
}

} // namespace EzTargets::RiscV64
