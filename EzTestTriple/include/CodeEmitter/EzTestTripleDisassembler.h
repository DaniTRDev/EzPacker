#ifndef EZPACKER_EZTESTTRIPLEDISASSEMBLER_H
#define EZPACKER_EZTESTTRIPLEDISASSEMBLER_H

#include "EzTestTripleCommon.h"
#include "InstructionSelector/EzTestTripleInstructionSet.h"

struct EzTestTripleDecodedInstruction
{
    uint64_t m_offset{ 0 };
    uint8_t m_opcodeId{ 0 };
    std::string m_mnemonic;
    uint8_t m_op0{ 0 };
    uint8_t m_op1{ 0 };
    uint8_t m_aux{ 0 };

    bool m_hasPayload32{ false };
    bool m_hasPayload64{ false };
    uint32_t m_payload32{ 0 };
    uint64_t m_payload64{ 0 };

    size_t m_instructionSize{ 4 };
    std::string m_disassemblyText;
};

class EzTestTripleDisassembler
{
  public:
    /**
     * Disassembles a single instruction starting at the given offset within the byte buffer.
     * Returns true if successfully decoded, false if buffer underflows or invalid data.
     */
    bool decodeInstruction(std::span<const uint8_t> code, uint64_t offset, EzTestTripleDecodedInstruction &outInst) const;

    /**
     * Disassembles an entire byte buffer and returns a vector of decoded instructions.
     */
    std::vector<EzTestTripleDecodedInstruction> disassembleBuffer(std::span<const uint8_t> code, uint64_t baseAddress = 0) const;

    /**
     * Formats and prints the disassembled buffer and returns it.
     */
    std::string dump(std::span<const uint8_t> code, uint64_t baseAddress = 0) const;
};

#endif // EZPACKER_EZTESTTRIPLEDISASSEMBLER_H