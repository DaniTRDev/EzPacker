#include "Encoding/RiscV64InstructionEncoder.h"

namespace EzTargets::RiscV64
{

namespace
{

void emitWord(std::vector<uint8_t> &out, uint32_t word)
{
    out.push_back(static_cast<uint8_t>(word & 0xFF));
    out.push_back(static_cast<uint8_t>((word >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((word >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((word >> 24) & 0xFF));
}

uint32_t readWord(std::span<const uint8_t> bytes, size_t offset)
{
    return static_cast<uint32_t>(bytes[offset]) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

void writeWord(std::span<uint8_t> bytes, size_t offset, uint32_t word)
{
    bytes[offset] = static_cast<uint8_t>(word & 0xFF);
    bytes[offset + 1] = static_cast<uint8_t>((word >> 8) & 0xFF);
    bytes[offset + 2] = static_cast<uint8_t>((word >> 16) & 0xFF);
    bytes[offset + 3] = static_cast<uint8_t>((word >> 24) & 0xFF);
}

uint32_t encodeBImm(int32_t offset)
{
    uint32_t imm = static_cast<uint32_t>(offset);
    uint32_t imm12 = (imm >> 12) & 0x1;
    uint32_t imm10_5 = (imm >> 5) & 0x3F;
    uint32_t imm4_1 = (imm >> 1) & 0xF;
    uint32_t imm11 = (imm >> 11) & 0x1;
    return (imm12 << 31) | (imm10_5 << 25) | (imm4_1 << 8) | (imm11 << 7);
}

uint32_t encodeJImm(int32_t offset)
{
    uint32_t imm = static_cast<uint32_t>(offset);
    uint32_t imm20 = (imm >> 20) & 0x1;
    uint32_t imm10_1 = (imm >> 1) & 0x3FF;
    uint32_t imm11 = (imm >> 11) & 0x1;
    uint32_t imm19_12 = (imm >> 12) & 0xFF;
    return (imm20 << 31) | (imm10_1 << 21) | (imm11 << 20) | (imm19_12 << 12);
}

} // namespace

bool InstructionEncoder::encode(const EncodingDesc &desc,
                                std::span<const ResolvedOperand> operands,
                                std::vector<uint8_t> &out,
                                EzCodeEmitter::EncodeResult &result)
{
    result = EzCodeEmitter::EncodeResult{};

    if (desc.m_form == EncForm::None)
    {
        return false;
    }

    // Handle pseudo-instructions first.
    if (desc.m_form == EncForm::Pseudo)
    {
        if (desc.m_opcode == 0 && desc.m_funct3 == 0 && desc.m_operandCount == 0)
        {
            // RET: jalr x0, ra, 0 (0x00008067)
            emitWord(out, 0x00008067);
            return true;
        }

        if (desc.m_opcode == 0x73)
        {
            if (desc.m_funct3 == 0 && desc.m_funct7 == 0x00)
            {
                emitWord(out, 0x00000073); // ecall
                return true;
            }
            if (desc.m_funct3 == 0 && desc.m_funct7 == 0x01)
            {
                emitWord(out, 0x00100073); // ebreak
                return true;
            }
            if (desc.m_funct3 == 0 && desc.m_funct7 == 0x02)
            {
                emitWord(out, 0x00200073); // uret
                return true;
            }
            if (desc.m_funct3 == 0x02 && desc.m_funct7 == 0x08)
            {
                emitWord(out, 0x10200073); // sret
                return true;
            }
            if (desc.m_funct3 == 0x02 && desc.m_funct7 == 0x18)
            {
                emitWord(out, 0x30200073); // mret
                return true;
            }
            if (desc.m_funct3 == 0x05 && desc.m_funct7 == 0x08)
            {
                emitWord(out, 0x10500073); // wfi
                return true;
            }
        }
        else if (desc.m_opcode == 0x0F)
        {
            if (desc.m_funct3 == 0)
            {
                emitWord(out, 0x0FF0000F); // fence
                return true;
            }
            if (desc.m_funct3 == 1)
            {
                emitWord(out, 0x0000100F); // fence.i
                return true;
            }
        }

        // CALL pseudo: jal ra, target
        if (desc.m_relocOperand != 0xFF || (!operands.empty() && operands[0].m_needsReloc))
        {
            result.m_hasReloc = true;
            result.m_relocOffset = 0;
            result.m_relocBits = 32;
            result.m_isBranch = true;
            // Emit JAL ra, 0 placeholder
            uint32_t jalRa = (1u << 7) | 0x6Fu;
            emitWord(out, jalRa);
            return true;
        }

        return false;
    }

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t rs3 = 0;
    int64_t imm = 0;
    bool hasReloc = false;

    for (uint8_t i = 0; i < desc.m_operandCount; ++i)
    {
        const auto &binding = desc.m_operands[i];
        if (binding.m_operandIndex >= operands.size())
        {
            continue;
        }

        const auto &op = operands[binding.m_operandIndex];
        switch (binding.m_slot)
        {
            case EncSlotKind::Rd:
                rd = op.m_reg & 0x1F;
                break;
            case EncSlotKind::Rs1:
                if (op.m_kind == ResolvedOperand::Kind::Memory)
                {
                    rs1 = op.m_mem.m_base & 0x1F;
                    imm = op.m_mem.m_disp;
                    if (op.m_mem.m_needsReloc)
                        hasReloc = true;
                }
                else
                {
                    rs1 = op.m_reg & 0x1F;
                }
                break;
            case EncSlotKind::Rs2:
                rs2 = op.m_reg & 0x1F;
                break;
            case EncSlotKind::Rs3:
                rs3 = op.m_reg & 0x1F;
                break;
            case EncSlotKind::Imm12Signed:
            case EncSlotKind::Imm12Store:
            case EncSlotKind::Shamt5:
            case EncSlotKind::Shamt6:
            case EncSlotKind::Csr:
                if (op.m_kind == ResolvedOperand::Kind::Memory)
                {
                    imm = op.m_mem.m_disp;
                    if (op.m_mem.m_needsReloc)
                        hasReloc = true;
                }
                else
                {
                    imm = op.m_imm;
                    if (op.m_needsReloc)
                        hasReloc = true;
                }
                break;
            case EncSlotKind::Imm12Branch:
            case EncSlotKind::Imm20Jump:
            case EncSlotKind::Rel:
                imm = op.m_imm;
                hasReloc = op.m_needsReloc || (binding.m_operandIndex == desc.m_relocOperand);
                break;
            case EncSlotKind::Imm20Upper:
                imm = op.m_imm;
                if (op.m_needsReloc)
                    hasReloc = true;
                break;
            default:
                break;
        }
    }

    uint32_t word = 0;
    switch (desc.m_form)
    {
        case EncForm::RType:
        {
            word = ((static_cast<uint32_t>(desc.m_funct7) & 0x7F) << 25) |
                   ((static_cast<uint32_t>(rs2) & 0x1F) << 20) |
                   ((static_cast<uint32_t>(rs1) & 0x1F) << 15) |
                   ((static_cast<uint32_t>(desc.m_funct3) & 0x7) << 12) |
                   ((static_cast<uint32_t>(rd) & 0x1F) << 7) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            break;
        }
        case EncForm::IType:
        {
            uint32_t imm12 = static_cast<uint32_t>(imm) & 0xFFF;
            word = (imm12 << 20) |
                   ((static_cast<uint32_t>(rs1) & 0x1F) << 15) |
                   ((static_cast<uint32_t>(desc.m_funct3) & 0x7) << 12) |
                   ((static_cast<uint32_t>(rd) & 0x1F) << 7) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            if (desc.m_funct7 != 0)
            {
                word |= ((static_cast<uint32_t>(desc.m_funct7) & 0x7F) << 25);
            }
            break;
        }
        case EncForm::SType:
        {
            uint32_t imm12 = static_cast<uint32_t>(imm) & 0xFFF;
            uint32_t imm11_5 = (imm12 >> 5) & 0x7F;
            uint32_t imm4_0 = imm12 & 0x1F;
            word = (imm11_5 << 25) |
                   ((static_cast<uint32_t>(rs2) & 0x1F) << 20) |
                   ((static_cast<uint32_t>(rs1) & 0x1F) << 15) |
                   ((static_cast<uint32_t>(desc.m_funct3) & 0x7) << 12) |
                   (imm4_0 << 7) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            break;
        }
        case EncForm::BType:
        {
            word = encodeBImm(static_cast<int32_t>(imm)) |
                   ((static_cast<uint32_t>(rs2) & 0x1F) << 20) |
                   ((static_cast<uint32_t>(rs1) & 0x1F) << 15) |
                   ((static_cast<uint32_t>(desc.m_funct3) & 0x7) << 12) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            break;
        }
        case EncForm::UType:
        {
            uint32_t uval = (static_cast<uint32_t>(imm) <= 0xFFFFF)
                                    ? ((static_cast<uint32_t>(imm) & 0xFFFFF) << 12)
                                    : (static_cast<uint32_t>(imm) & 0xFFFFF000);
            word = uval |
                   ((static_cast<uint32_t>(rd) & 0x1F) << 7) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            break;
        }
        case EncForm::JType:
        {
            word = encodeJImm(static_cast<int32_t>(imm)) |
                   ((static_cast<uint32_t>(rd) & 0x1F) << 7) |
                   (static_cast<uint32_t>(desc.m_opcode) & 0x7F);
            break;
        }
        default:
            return false;
    }

    if (hasReloc)
    {
        result.m_hasReloc = true;
        result.m_relocOffset = 0;
        result.m_relocBits = 32;
        result.m_isBranch = (desc.m_form == EncForm::BType || desc.m_form == EncForm::JType);
    }

    emitWord(out, word);
    return true;
}

void InstructionEncoder::encodeRegisterMove(const ResolvedOperand &dst,
                                            const ResolvedOperand &src,
                                            std::vector<uint8_t> &out)
{
    if (dst.m_isVr || src.m_isVr)
    {
        // vmv.v.v vd, vs1 (opcode 0x57, funct3 0, funct7 0x2F, vs2 = 0)
        uint32_t rd = dst.m_reg & 0x1F;
        uint32_t rs = src.m_reg & 0x1F;
        uint32_t word = (0x2Fu << 25) | (0u << 20) | (rs << 15) | (0u << 12) | (rd << 7) | 0x57u;
        emitWord(out, word);
    }
    else if (dst.m_isFpr || src.m_isFpr)
    {
        uint32_t rd = dst.m_reg & 0x1F;
        uint32_t rs = src.m_reg & 0x1F;
        uint32_t funct7 = (dst.m_sizeBytes <= 4 && src.m_sizeBytes <= 4) ? 0x10u : 0x11u;
        // fsgnj.s or fsgnj.d rd, rs1, rs1 (opcode 0x53, funct3 0, funct7, rs2 = rs1)
        uint32_t word = (funct7 << 25) | (rs << 20) | (rs << 15) | (0u << 12) | (rd << 7) | 0x53u;
        emitWord(out, word);
    }
    else
    {
        // addi rd, rs, 0 (opcode 0x13, funct3 0, imm 0)
        uint32_t rd = dst.m_reg & 0x1F;
        uint32_t rs = src.m_reg & 0x1F;
        uint32_t word = (0u << 20) | (rs << 15) | (0u << 12) | (rd << 7) | 0x13u;
        emitWord(out, word);
    }
}

void InstructionEncoder::emitJal(std::vector<uint8_t> &out, uint8_t rd, int32_t offset)
{
    uint32_t word = encodeJImm(offset) | ((static_cast<uint32_t>(rd) & 0x1F) << 7) | 0x6Fu;
    emitWord(out, word);
}

void InstructionEncoder::emitBranch(std::vector<uint8_t> &out, uint8_t funct3, uint8_t rs1, uint8_t rs2, int32_t offset)
{
    uint32_t word = encodeBImm(offset) |
                    ((static_cast<uint32_t>(rs2) & 0x1F) << 20) |
                    ((static_cast<uint32_t>(rs1) & 0x1F) << 15) |
                    ((static_cast<uint32_t>(funct3) & 0x7) << 12) |
                    0x63u;
    emitWord(out, word);
}

bool InstructionEncoder::classifyBranch(std::span<const uint8_t> bytes,
                                        size_t offset,
                                        bool &isJal,
                                        bool &isBranch,
                                        uint8_t &funct3,
                                        uint8_t &rs1,
                                        uint8_t &rs2,
                                        uint8_t &rd)
{
    if (offset + 4 > bytes.size())
    {
        return false;
    }

    uint32_t word = readWord(bytes, offset);
    uint8_t opcode = static_cast<uint8_t>(word & 0x7F);

    if (opcode == 0x6F) // JAL
    {
        isJal = true;
        isBranch = false;
        rd = static_cast<uint8_t>((word >> 7) & 0x1F);
        funct3 = 0;
        rs1 = 0;
        rs2 = 0;
        return true;
    }

    if (opcode == 0x63) // BRANCH
    {
        isJal = false;
        isBranch = true;
        funct3 = static_cast<uint8_t>((word >> 12) & 0x7);
        rs1 = static_cast<uint8_t>((word >> 15) & 0x1F);
        rs2 = static_cast<uint8_t>((word >> 20) & 0x1F);
        rd = 0;
        return true;
    }

    return false;
}

bool InstructionEncoder::patchBranchDisp(std::span<uint8_t> bytes, size_t offset, int32_t disp)
{
    if (offset + 4 > bytes.size())
    {
        return false;
    }

    uint32_t word = readWord(bytes, offset);
    // Clear B-type immediate bits: bit 31, [30:25], [11:8], bit 7
    word &= ~0xFE000F80u;
    word |= encodeBImm(disp);
    writeWord(bytes, offset, word);
    return true;
}

bool InstructionEncoder::patchJalDisp(std::span<uint8_t> bytes, size_t offset, int32_t disp)
{
    if (offset + 4 > bytes.size())
    {
        return false;
    }

    uint32_t word = readWord(bytes, offset);
    // Clear J-type immediate bits: bit 31, [30:21], bit 20, [19:12]
    word &= ~0xFFFFF000u;
    word |= encodeJImm(disp);
    writeWord(bytes, offset, word);
    return true;
}

bool InstructionEncoder::writeDisp32(std::span<uint8_t> bytes, size_t offset, int32_t value)
{
    if (offset + 4 > bytes.size())
    {
        return false;
    }
    writeWord(bytes, offset, static_cast<uint32_t>(value));
    return true;
}

} // namespace EzTargets::RiscV64
