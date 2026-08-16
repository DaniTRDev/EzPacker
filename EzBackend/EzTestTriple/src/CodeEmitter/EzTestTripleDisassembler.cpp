#include "CodeEmitter/EzTestTripleDisassembler.h"

static uint32_t ReadU32LE(const uint8_t *ptr)
{
    return static_cast<uint32_t>(ptr[0]) | (static_cast<uint32_t>(ptr[1]) << 8) |
            (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
}

static uint64_t ReadU64LE(const uint8_t *ptr)
{
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i)
    {
        val |= (static_cast<uint64_t>(ptr[i]) << (i * 8));
    }
    return val;
}

bool EzTestTripleDisassembler::decodeInstruction(std::span<const uint8_t> code,
                                                 uint64_t offset,
                                                 EzTestTripleDecodedInstruction &outInst) const
{
    if (offset + 4 > code.size())
    {
        return false;
    }

    const uint8_t *ptr = code.data() + offset;
    uint8_t opcodeId = ptr[0];

    const MirTargetInstructionDesc *desc = EzTestTriple::TargetInst::getById(opcodeId);
    if (!desc)
    {
        return false;
    }

    outInst.m_offset = offset;
    outInst.m_opcodeId = opcodeId;
    outInst.m_op0 = ptr[1];
    outInst.m_op1 = ptr[2];
    outInst.m_aux = ptr[3];
    outInst.m_mnemonic = desc->getName();
    outInst.m_hasPayload32 = false;
    outInst.m_hasPayload64 = false;
    outInst.m_instructionSize = 4;

    const uint8_t id = outInst.m_opcodeId;
    const uint8_t aux = outInst.m_aux;

    // 1. Full 64-bit Payload Instructions
    if (id == 11 /* MOVABS64ri */)
    {
        if (offset + 12 > code.size())
            return false;
        outInst.m_hasPayload64 = true;
        outInst.m_payload64 = ReadU64LE(ptr + 4);
        outInst.m_instructionSize = 12;
        outInst.m_disassemblyText =
                std::format("{:<14} r{}, 0x{:016X}", outInst.m_mnemonic, outInst.m_op0, outInst.m_payload64);
        return true;
    }

    // 2. Base + Displacement Memory Instructions
    if (aux == 0x02)
    {
        if (offset + 8 > code.size())
            return false;
        outInst.m_hasPayload32 = true;
        outInst.m_payload32 = ReadU32LE(ptr + 4);
        outInst.m_instructionSize = 8;
        int32_t disp = static_cast<int32_t>(outInst.m_payload32);

        // Check if store (MOV*mr: op0=mem, op1=reg) or load (MOV*rm: op0=reg, op1=mem)
        if ((id >= 26 && id <= 35))
        {
            outInst.m_disassemblyText =
                    std::format("{:<14} [r{} + 0x{:X}], r{}", outInst.m_mnemonic, outInst.m_op0, disp, outInst.m_op1);
        }
        else
        {
            outInst.m_disassemblyText =
                    std::format("{:<14} r{}, [r{} + 0x{:X}]", outInst.m_mnemonic, outInst.m_op0, outInst.m_op1, disp);
        }
        return true;
    }

    // 3. 32-bit Immediate / RIP / Branch Payload Instructions
    const bool isImm32 = (id >= 7 && id <= 10) || // MOV*ri
            (id >= 46 && id <= 49) ||             // ADD*ri
            (id >= 54 && id <= 57) ||             // ADC*ri
            (id >= 62 && id <= 65) ||             // SUB*ri
            (id >= 70 && id <= 73) ||             // SBB*ri
            (id >= 78 && id <= 81) ||             // AND*ri
            (id >= 86 && id <= 89) ||             // OR*ri
            (id >= 94 && id <= 97) ||             // XOR*ri
            (id >= 102 && id <= 105) ||           // CMP*ri
            (id >= 110 && id <= 113) ||           // TEST*ri
            (id >= 122 && id <= 124) ||           // IMUL*rri
            (id >= 145 && id <= 148) ||           // SHL*ri
            (id >= 153 && id <= 156) ||           // SHR*ri
            (id >= 161 && id <= 164) ||           // SAR*ri
            (id >= 204 && id <= 212) ||           // Jcc & JMP
            (id == 215) ||                        // CALL
            (id == 218) ||                        // PUSH64i32
            (aux == 0x01);                        // RIP-relative

    if (isImm32)
    {
        if (offset + 8 > code.size())
            return false;
        outInst.m_hasPayload32 = true;
        outInst.m_payload32 = ReadU32LE(ptr + 4);
        outInst.m_instructionSize = 8;
        int32_t immVal = static_cast<int32_t>(outInst.m_payload32);

        if (aux == 0x01)
        {
            outInst.m_disassemblyText =
                    std::format("{:<14} r{}, [rip + 0x{:X}]", outInst.m_mnemonic, outInst.m_op0, immVal);
        }
        else if (id >= 204 && id <= 212) // Branches
        {
            uint64_t targetAddr = offset + 8 + immVal;
            outInst.m_disassemblyText = std::format("{:<14} 0x{:04X} ({:+d})", outInst.m_mnemonic, targetAddr, immVal);
        }
        else if (id == 215) // CALL
        {
            outInst.m_disassemblyText =
                    std::format("{:<14} <func#{} / offset {:+d}>", outInst.m_mnemonic, outInst.m_payload32, immVal);
        }
        else
        {
            outInst.m_disassemblyText = std::format("{:<14} r{}, 0x{:X}", outInst.m_mnemonic, outInst.m_op0, immVal);
        }
        return true;
    }

    // 4. Pure Register-Register / Unary / System Instructions (4 Bytes)
    if (id == 216 || id == 220 || id == 221 || id == 222) // RET, NOP, HLT, SYSCALL
    {
        outInst.m_disassemblyText = outInst.m_mnemonic;
    }
    else if ((id >= 114 && id <= 118) || (id >= 125 && id <= 140) || id == 217 || id == 219) // Unary / 1-op
    {
        outInst.m_disassemblyText = std::format("{:<14} r{}", outInst.m_mnemonic, outInst.m_op0);
    }
    else // Standard 2-operand reg-reg
    {
        outInst.m_disassemblyText = std::format("{:<14} r{}, r{}", outInst.m_mnemonic, outInst.m_op0, outInst.m_op1);
    }

    return true;
}

std::vector<EzTestTripleDecodedInstruction> EzTestTripleDisassembler::disassembleBuffer(std::span<const uint8_t> code,
                                                                                        uint64_t baseAddress) const
{
    std::vector<EzTestTripleDecodedInstruction> instructions;
    uint64_t cursor = 0;

    while (cursor < code.size())
    {
        EzTestTripleDecodedInstruction inst;
        if (decodeInstruction(code, cursor, inst))
        {
            inst.m_offset += baseAddress;
            instructions.push_back(std::move(inst));
            cursor += inst.m_instructionSize;
        }
        else
        {
            // Skip unaligned or padding byte (e.g. 0x90 NOP padding between functions)
            if (code[cursor] == 0x90)
            {
                EzTestTripleDecodedInstruction nopInst;
                nopInst.m_offset = cursor + baseAddress;
                nopInst.m_mnemonic = "nop (pad)";
                nopInst.m_disassemblyText = "nop (alignment pad)";
                nopInst.m_instructionSize = 1;
                instructions.push_back(nopInst);
                cursor += 1;
                continue;
            }
            break;
        }
    }
    return instructions;
}

std::string EzTestTripleDisassembler::dump(std::span<const uint8_t> code, uint64_t baseAddress) const
{
    auto insts = disassembleBuffer(code, baseAddress);

    std::string out;
    out.reserve(insts.size() * 64 + 256); // Pre-allocate sensible buffer capacity

    out += "============================== DISASSEMBLY ==============================\n";
    out += "  Offset   | Bytes                 | Assembly\n";
    out += "-----------+-----------------------+-------------------------------------\n";

    for (const auto &inst : insts)
    {
        // Format Raw Bytes in Hex
        std::string rawBytes;
        for (size_t i = 0; i < inst.m_instructionSize && (inst.m_offset - baseAddress + i) < code.size(); ++i)
        {
            std::format_to(std::back_inserter(rawBytes), "{:02X} ", code[inst.m_offset - baseAddress + i]);
        }

        std::format_to(std::back_inserter(out),
                       "  0x{:06X} | {:<21} | {}\n",
                       inst.m_offset,
                       rawBytes,
                       inst.m_disassemblyText);
    }

    out += "=========================================================================\n";
    return out;
}