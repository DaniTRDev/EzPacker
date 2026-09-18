#include "X86_64/X86_64Encoding.h"

namespace EzCodeEmitter::X86_64
{

std::string_view getRegName(Reg reg, uint8_t sizeInBytes)
{
    static constexpr std::string_view names64[] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
    };
    static constexpr std::string_view names32[] = {
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
    };
    static constexpr std::string_view names16[] = {
        "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
    };
    static constexpr std::string_view names8[] = {
        "al",  "cl",  "dl",  "bl",  "spl", "bpl", "sil", "dil",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
    };

    static constexpr std::string_view namesXmm[] = {
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
    };

    uint8_t id = getRegId(reg);
    if (id >= 16 && id <= 31)
    {
        return namesXmm[id - 16];
    }
    if (id >= 16)
    {
        return "none";
    }

    switch (sizeInBytes)
    {
        case 1: return names8[id];
        case 2: return names16[id];
        case 4: return names32[id];
        case 8: return names64[id];
        default: return names64[id];
    }
}

namespace
{

uint8_t scaleToPower(uint8_t scale)
{
    switch (scale)
    {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        default: return 0;
    }
}

void emitImm8(std::vector<uint8_t> &out, int8_t imm)
{
    out.push_back(static_cast<uint8_t>(imm));
}

void emitImm16(std::vector<uint8_t> &out, int16_t imm)
{
    out.push_back(static_cast<uint8_t>(imm & 0xFF));
    out.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
}

void emitImm32(std::vector<uint8_t> &out, int32_t imm)
{
    out.push_back(static_cast<uint8_t>(imm & 0xFF));
    out.push_back(static_cast<uint8_t>((imm >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((imm >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((imm >> 24) & 0xFF));
}

void emitImm64(std::vector<uint8_t> &out, int64_t imm)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(static_cast<uint8_t>((imm >> (i * 8)) & 0xFF));
    }
}

} // namespace

void InstructionEncoder::encodeModRMSIB(std::vector<uint8_t> &out,
                                       RexPrefix &rex,
                                       uint8_t regOrOpcodeDigit,
                                       const MemoryOperand &mem)
{
    // rex.r is for the reg field in ModR/M
    uint8_t regBits = regOrOpcodeDigit & 0x07;

    switch (mem.m_kind)
    {
        case MemoryOperand::Kind::RipRel:
        {
            // RIP-relative: mod=00, rm=5 (101b)
            out.push_back(encodeModRM(0, regBits, 5));
            emitImm32(out, static_cast<int32_t>(mem.m_disp));
            break;
        }
        case MemoryOperand::Kind::BaseDisp:
        {
            uint8_t baseLow = getLow3Bits(mem.m_base);
            rex.b = isExtendedReg(mem.m_base);

            bool needSib = (baseLow == 4); // RSP or R12 requires SIB
            uint8_t mod = 0;
            bool emitDisp8 = false;
            bool emitDisp32 = false;

            if (mem.m_disp == 0)
            {
                if (baseLow == 5) // RBP or R13 with disp 0 requires mod=01, disp8=0
                {
                    mod = 1;
                    emitDisp8 = true;
                }
                else
                {
                    mod = 0;
                }
            }
            else if (mem.m_disp >= -128 && mem.m_disp <= 127)
            {
                mod = 1;
                emitDisp8 = true;
            }
            else
            {
                mod = 2;
                emitDisp32 = true;
            }

            uint8_t rmBits = needSib ? 4 : baseLow;
            out.push_back(encodeModRM(mod, regBits, rmBits));

            if (needSib)
            {
                // SIB: scale=0, index=4 (none), base=baseLow
                out.push_back(encodeSIB(0, 4, baseLow));
            }

            if (emitDisp8)
            {
                emitImm8(out, static_cast<int8_t>(mem.m_disp));
            }
            else if (emitDisp32)
            {
                emitImm32(out, static_cast<int32_t>(mem.m_disp));
            }
            break;
        }
        case MemoryOperand::Kind::BaseIndexScaleDisp:
        {
            if (mem.m_base == Reg::None)
            {
                // [index*scale + disp32] (no base)
                rex.x = isExtendedReg(mem.m_index);
                out.push_back(encodeModRM(0, regBits, 4)); // mod=00, rm=4 (SIB)
                uint8_t indexLow = getLow3Bits(mem.m_index);
                out.push_back(encodeSIB(scaleToPower(mem.m_scale), indexLow, 5)); // base=5 (disp32)
                emitImm32(out, static_cast<int32_t>(mem.m_disp));
            }
            else
            {
                uint8_t baseLow = getLow3Bits(mem.m_base);
                uint8_t indexLow = getLow3Bits(mem.m_index);
                rex.b = isExtendedReg(mem.m_base);
                rex.x = isExtendedReg(mem.m_index);

                uint8_t mod = 0;
                bool emitDisp8 = false;
                bool emitDisp32 = false;

                if (mem.m_disp == 0)
                {
                    if (baseLow == 5) // RBP or R13 with disp 0 requires mod=01
                    {
                        mod = 1;
                        emitDisp8 = true;
                    }
                    else
                    {
                        mod = 0;
                    }
                }
                else if (mem.m_disp >= -128 && mem.m_disp <= 127)
                {
                    mod = 1;
                    emitDisp8 = true;
                }
                else
                {
                    mod = 2;
                    emitDisp32 = true;
                }

                out.push_back(encodeModRM(mod, regBits, 4)); // rm=4 indicates SIB
                out.push_back(encodeSIB(scaleToPower(mem.m_scale), indexLow, baseLow));

                if (emitDisp8)
                {
                    emitImm8(out, static_cast<int8_t>(mem.m_disp));
                }
                else if (emitDisp32)
                {
                    emitImm32(out, static_cast<int32_t>(mem.m_disp));
                }
            }
            break;
        }
    }
}

// =========================================================================
// MOV Implementation
// =========================================================================

void InstructionEncoder::emitMovRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(src);
    rex.b = isExtendedReg(dst);

    // Byte register encoding: SPL, BPL, SIL, DIL require REX prefix
    if (size == 1 && (getRegId(dst) >= 4 || getRegId(src) >= 4))
    {
        // Force REX presence even if W/R/B are zero
        // In 64-bit mode, 0x40 specifies new byte registers
    }

    if (size == 2)
    {
        out.push_back(0x66); // Operand size override prefix
    }

    if (rex.isNeeded() || (size == 1 && (getRegId(dst) >= 4 || getRegId(src) >= 4)))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0x88 : 0x89); // MOV mr (src is reg field, dst is rm field)
    out.push_back(encodeModRM(3, getLow3Bits(src), getLow3Bits(dst)));
}

void InstructionEncoder::emitMovRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(dst);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(dst), src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(dst) >= 4))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0x8A : 0x8B); // MOV rm
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitMovMR(std::vector<uint8_t> &out, const MemoryOperand &dst, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(src);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(src), dst);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(src) >= 4))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0x88 : 0x89); // MOV mr
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitMovRI(std::vector<uint8_t> &out, Reg dst, int64_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.b = isExtendedReg(dst);

    if (size == 8)
    {
        // Check if fits in signed 32-bit: use C7 /0 imm32 (sign extended)
        if (imm >= -2147483648LL && imm <= 2147483647LL)
        {
            rex.w = true;
            out.push_back(rex.encode());
            out.push_back(0xC7);
            out.push_back(encodeModRM(3, 0, getLow3Bits(dst)));
            emitImm32(out, static_cast<int32_t>(imm));
        }
        else
        {
            // 64-bit immediate (MOVABS): REX.W + B8+rd imm64
            rex.w = true;
            out.push_back(rex.encode());
            out.push_back(0xB8 + getLow3Bits(dst));
            emitImm64(out, imm);
        }
    }
    else if (size == 4)
    {
        // 32-bit immediate: B8+rd imm32 (clears top 32 bits of 64-bit reg)
        if (rex.b)
        {
            out.push_back(rex.encode());
        }
        out.push_back(0xB8 + getLow3Bits(dst));
        emitImm32(out, static_cast<int32_t>(imm));
    }
    else if (size == 2)
    {
        out.push_back(0x66);
        if (rex.b)
        {
            out.push_back(rex.encode());
        }
        out.push_back(0xB8 + getLow3Bits(dst));
        emitImm16(out, static_cast<int16_t>(imm));
    }
    else if (size == 1)
    {
        if (rex.b || getRegId(dst) >= 4)
        {
            out.push_back(rex.encode());
        }
        out.push_back(0xB0 + getLow3Bits(dst));
        emitImm8(out, static_cast<int8_t>(imm));
    }
}

void InstructionEncoder::emitMovMI(std::vector<uint8_t> &out, const MemoryOperand &dst, int32_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, 0, dst); // /0 digit

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    if (size == 1)
    {
        out.push_back(0xC6);
        out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else
    {
        out.push_back(0xC7);
        out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
        if (size == 2)
        {
            emitImm16(out, static_cast<int16_t>(imm));
        }
        else
        {
            emitImm32(out, imm);
        }
    }
}

void InstructionEncoder::emitMovzxRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize)
{
    RexPrefix rex;
    rex.w = true; // Destination is 64-bit
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);

    out.push_back(rex.encode());
    out.push_back(0x0F);
    out.push_back(srcSize == 1 ? 0xB6 : 0xB7);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitMovzxRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t srcSize)
{
    RexPrefix rex;
    rex.w = true;
    rex.r = isExtendedReg(dst);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(dst), src);

    out.push_back(rex.encode());
    out.push_back(0x0F);
    out.push_back(srcSize == 1 ? 0xB6 : 0xB7);
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitMovsxRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize)
{
    RexPrefix rex;
    rex.w = true;
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);

    out.push_back(rex.encode());
    if (srcSize == 4)
    {
        // MOVSXD: 0x63
        out.push_back(0x63);
    }
    else
    {
        out.push_back(0x0F);
        out.push_back(srcSize == 1 ? 0xBE : 0xBF);
    }
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitMovsxRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t srcSize)
{
    RexPrefix rex;
    rex.w = true;
    rex.r = isExtendedReg(dst);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(dst), src);

    out.push_back(rex.encode());
    if (srcSize == 4)
    {
        out.push_back(0x63);
    }
    else
    {
        out.push_back(0x0F);
        out.push_back(srcSize == 1 ? 0xBE : 0xBF);
    }
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

// =========================================================================
// ALU Implementation
// =========================================================================

namespace
{

uint8_t getAluPrimaryOpcode(AluOp op, bool isMR)
{
    // MR opcodes: ADD=0x01, OR=0x09, ADC=0x11, SBB=0x19, AND=0x21, SUB=0x29, XOR=0x31, CMP=0x39
    // RM opcodes: ADD=0x03, OR=0x0B, ADC=0x13, SBB=0x1B, AND=0x23, SUB=0x2B, XOR=0x33, CMP=0x3B
    uint8_t base = static_cast<uint8_t>(op) * 8;
    return isMR ? (base + 1) : (base + 3);
}

uint8_t getAluDigit(AluOp op)
{
    return static_cast<uint8_t>(op);
}

} // namespace

void InstructionEncoder::emitAluRR(std::vector<uint8_t> &out, AluOp op, Reg dst, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(src);
    rex.b = isExtendedReg(dst);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    uint8_t opcode = getAluPrimaryOpcode(op, /*isMR=*/true);
    if (size == 1)
    {
        opcode -= 1; // 0x00, 0x08, etc. for 8-bit
    }

    out.push_back(opcode);
    out.push_back(encodeModRM(3, getLow3Bits(src), getLow3Bits(dst)));
}

void InstructionEncoder::emitAluRM(std::vector<uint8_t> &out, AluOp op, Reg dst, const MemoryOperand &src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(dst);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(dst), src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    uint8_t opcode = getAluPrimaryOpcode(op, /*isMR=*/false);
    if (size == 1)
    {
        opcode -= 1;
    }

    out.push_back(opcode);
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitAluMR(std::vector<uint8_t> &out, AluOp op, const MemoryOperand &dst, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(src);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(src), dst);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    uint8_t opcode = getAluPrimaryOpcode(op, /*isMR=*/true);
    if (size == 1)
    {
        opcode -= 1;
    }

    out.push_back(opcode);
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitAluRI(std::vector<uint8_t> &out, AluOp op, Reg dst, int32_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(dst);

    bool fitsInImm8 = (imm >= -128 && imm <= 127);
    uint8_t digit = getAluDigit(op);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    if (size == 1)
    {
        out.push_back(0x80);
        out.push_back(encodeModRM(3, digit, getLow3Bits(dst)));
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else if (fitsInImm8)
    {
        // 0x83 /digit imm8 (sign extended to 16/32/64 bits)
        out.push_back(0x83);
        out.push_back(encodeModRM(3, digit, getLow3Bits(dst)));
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else
    {
        // 0x81 /digit imm32
        out.push_back(0x81);
        out.push_back(encodeModRM(3, digit, getLow3Bits(dst)));
        if (size == 2)
        {
            emitImm16(out, static_cast<int16_t>(imm));
        }
        else
        {
            emitImm32(out, imm);
        }
    }
}

void InstructionEncoder::emitAluMI(std::vector<uint8_t> &out, AluOp op, const MemoryOperand &dst, int32_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);

    uint8_t digit = getAluDigit(op);
    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, digit, dst);

    bool fitsInImm8 = (imm >= -128 && imm <= 127);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    if (size == 1)
    {
        out.push_back(0x80);
        out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else if (fitsInImm8)
    {
        out.push_back(0x83);
        out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else
    {
        out.push_back(0x81);
        out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
        if (size == 2)
        {
            emitImm16(out, static_cast<int16_t>(imm));
        }
        else
        {
            emitImm32(out, imm);
        }
    }
}

// =========================================================================
// TEST & LEA
// =========================================================================

void InstructionEncoder::emitTestRR(std::vector<uint8_t> &out, Reg r1, Reg r2, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(r2);
    rex.b = isExtendedReg(r1);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0x84 : 0x85);
    out.push_back(encodeModRM(3, getLow3Bits(r2), getLow3Bits(r1)));
}

void InstructionEncoder::emitTestRI(std::vector<uint8_t> &out, Reg r, int32_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(r);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    if (size == 1)
    {
        out.push_back(0xF6);
        out.push_back(encodeModRM(3, 0, getLow3Bits(r)));
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else
    {
        out.push_back(0xF7);
        out.push_back(encodeModRM(3, 0, getLow3Bits(r)));
        if (size == 2)
        {
            emitImm16(out, static_cast<int16_t>(imm));
        }
        else
        {
            emitImm32(out, imm);
        }
    }
}

void InstructionEncoder::emitLea(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(dst);

    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, getLow3Bits(dst), src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    out.push_back(0x8D); // LEA opcode
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

// =========================================================================
// PUSH / POP
// =========================================================================

void InstructionEncoder::emitPushR(std::vector<uint8_t> &out, Reg reg)
{
    if (isExtendedReg(reg))
    {
        out.push_back(0x41); // REX.B
    }
    out.push_back(0x50 + getLow3Bits(reg));
}

void InstructionEncoder::emitPushImm8(std::vector<uint8_t> &out, int8_t imm)
{
    out.push_back(0x6A);
    emitImm8(out, imm);
}

void InstructionEncoder::emitPushImm32(std::vector<uint8_t> &out, int32_t imm)
{
    out.push_back(0x68);
    emitImm32(out, imm);
}

void InstructionEncoder::emitPushM(std::vector<uint8_t> &out, const MemoryOperand &mem)
{
    RexPrefix rex;
    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, 6, mem); // /6 digit for PUSH m64

    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0xFF);
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

void InstructionEncoder::emitPopR(std::vector<uint8_t> &out, Reg reg)
{
    if (isExtendedReg(reg))
    {
        out.push_back(0x41); // REX.B
    }
    out.push_back(0x58 + getLow3Bits(reg));
}

void InstructionEncoder::emitPopM(std::vector<uint8_t> &out, const MemoryOperand &mem)
{
    RexPrefix rex;
    std::vector<uint8_t> modrmBytes;
    encodeModRMSIB(modrmBytes, rex, 0, mem); // /0 digit for POP m64

    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0x8F);
    out.insert(out.end(), modrmBytes.begin(), modrmBytes.end());
}

// =========================================================================
// Control Flow Implementation
// =========================================================================

void InstructionEncoder::emitJmpShort(std::vector<uint8_t> &out, int8_t disp)
{
    out.push_back(0xEB);
    emitImm8(out, disp);
}

void InstructionEncoder::emitJmpNear(std::vector<uint8_t> &out, int32_t disp)
{
    out.push_back(0xE9);
    emitImm32(out, disp);
}

void InstructionEncoder::emitJmpR(std::vector<uint8_t> &out, Reg reg)
{
    if (isExtendedReg(reg))
    {
        out.push_back(0x41); // REX.B
    }
    out.push_back(0xFF);
    out.push_back(encodeModRM(3, 4, getLow3Bits(reg))); // /4 digit
}

void InstructionEncoder::emitJccShort(std::vector<uint8_t> &out, ConditionCode cc, int8_t disp)
{
    out.push_back(0x70 | static_cast<uint8_t>(cc));
    emitImm8(out, disp);
}

void InstructionEncoder::emitJccNear(std::vector<uint8_t> &out, ConditionCode cc, int32_t disp)
{
    out.push_back(0x0F);
    out.push_back(0x80 | static_cast<uint8_t>(cc));
    emitImm32(out, disp);
}

void InstructionEncoder::emitCallNear(std::vector<uint8_t> &out, int32_t disp)
{
    out.push_back(0xE8);
    emitImm32(out, disp);
}

void InstructionEncoder::emitCallR(std::vector<uint8_t> &out, Reg reg)
{
    if (isExtendedReg(reg))
    {
        out.push_back(0x41);
    }
    out.push_back(0xFF);
    out.push_back(encodeModRM(3, 2, getLow3Bits(reg))); // /2 digit
}

void InstructionEncoder::emitRet(std::vector<uint8_t> &out)
{
    out.push_back(0xC3);
}

void InstructionEncoder::emitRetImm(std::vector<uint8_t> &out, uint16_t imm)
{
    out.push_back(0xC2);
    emitImm16(out, static_cast<int16_t>(imm));
}

void InstructionEncoder::emitNop(std::vector<uint8_t> &out, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        out.push_back(0x90);
    }
}

// =========================================================================
// Shift Instructions (SHL, SHR, SAR)
// =========================================================================

namespace
{

void emitShiftRIImpl(std::vector<uint8_t> &out, uint8_t digit, Reg dst, uint8_t amt, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(dst);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(dst) >= 4 && getRegId(dst) <= 15))
    {
        out.push_back(rex.encode());
    }

    if (amt == 1)
    {
        out.push_back(size == 1 ? 0xD0 : 0xD1);
        out.push_back(InstructionEncoder::encodeModRM(3, digit, getLow3Bits(dst)));
    }
    else
    {
        out.push_back(size == 1 ? 0xC0 : 0xC1);
        out.push_back(InstructionEncoder::encodeModRM(3, digit, getLow3Bits(dst)));
        emitImm8(out, static_cast<int8_t>(amt));
    }
}

void emitShiftRCLImpl(std::vector<uint8_t> &out, uint8_t digit, Reg dst, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(dst);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(dst) >= 4 && getRegId(dst) <= 15))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0xD2 : 0xD3);
    out.push_back(InstructionEncoder::encodeModRM(3, digit, getLow3Bits(dst)));
}

void emitUnaryRImpl(std::vector<uint8_t> &out, uint8_t digit, Reg reg, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(reg);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(reg) >= 4 && getRegId(reg) <= 15))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0xF6 : 0xF7);
    out.push_back(InstructionEncoder::encodeModRM(3, digit, getLow3Bits(reg)));
}

void emitSseOpRR(std::vector<uint8_t> &out, uint8_t prefixByte, uint8_t opcode1, uint8_t opcode2, Reg dst, Reg src)
{
    if (prefixByte != 0)
    {
        out.push_back(prefixByte);
    }
    RexPrefix rex;
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    if (opcode1 != 0)
    {
        out.push_back(opcode1);
    }
    out.push_back(opcode2);
    out.push_back(InstructionEncoder::encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

} // namespace

void InstructionEncoder::emitShlRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size)
{
    emitShiftRIImpl(out, 4, dst, amt, size);
}

void InstructionEncoder::emitShlRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size)
{
    emitShiftRCLImpl(out, 4, dst, size);
}

void InstructionEncoder::emitShrRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size)
{
    emitShiftRIImpl(out, 5, dst, amt, size);
}

void InstructionEncoder::emitShrRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size)
{
    emitShiftRCLImpl(out, 5, dst, size);
}

void InstructionEncoder::emitSarRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size)
{
    emitShiftRIImpl(out, 7, dst, amt, size);
}

void InstructionEncoder::emitSarRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size)
{
    emitShiftRCLImpl(out, 7, dst, size);
}

// =========================================================================
// Unary Arithmetic (NEG, NOT)
// =========================================================================

void InstructionEncoder::emitNegR(std::vector<uint8_t> &out, Reg reg, uint8_t size)
{
    emitUnaryRImpl(out, 3, reg, size);
}

void InstructionEncoder::emitNotR(std::vector<uint8_t> &out, Reg reg, uint8_t size)
{
    emitUnaryRImpl(out, 2, reg, size);
}

// =========================================================================
// Multiply & Divide (IMUL, IDIV, DIV)
// =========================================================================

void InstructionEncoder::emitImulRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    out.push_back(0x0F);
    out.push_back(0xAF);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitImulRI(std::vector<uint8_t> &out, Reg dst, Reg src, int32_t imm, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    if (imm >= -128 && imm <= 127)
    {
        out.push_back(0x6B);
        out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
        emitImm8(out, static_cast<int8_t>(imm));
    }
    else
    {
        out.push_back(0x69);
        out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
        if (size == 2)
        {
            emitImm16(out, static_cast<int16_t>(imm));
        }
        else
        {
            emitImm32(out, imm);
        }
    }
}

void InstructionEncoder::emitIdivR(std::vector<uint8_t> &out, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(src) >= 4 && getRegId(src) <= 15))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0xF6 : 0xF7);
    out.push_back(encodeModRM(3, 7, getLow3Bits(src)));
}

void InstructionEncoder::emitDivR(std::vector<uint8_t> &out, Reg src, uint8_t size)
{
    RexPrefix rex;
    rex.w = (size == 8);
    rex.b = isExtendedReg(src);

    if (size == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || (size == 1 && getRegId(src) >= 4 && getRegId(src) <= 15))
    {
        out.push_back(rex.encode());
    }

    out.push_back(size == 1 ? 0xF6 : 0xF7);
    out.push_back(encodeModRM(3, 6, getLow3Bits(src)));
}

// =========================================================================
// Conditional Set (SETcc)
// =========================================================================

void InstructionEncoder::emitSetcc(std::vector<uint8_t> &out, ConditionCode cc, Reg dst)
{
    RexPrefix rex;
    rex.b = isExtendedReg(dst);

    if (rex.isNeeded() || (getRegId(dst) >= 4 && getRegId(dst) <= 15))
    {
        out.push_back(rex.encode());
    }

    out.push_back(0x0F);
    out.push_back(0x90 + static_cast<uint8_t>(cc));
    out.push_back(encodeModRM(3, 0, getLow3Bits(dst)));
}

// =========================================================================
// Floating Point / SSE Instructions
// =========================================================================

void InstructionEncoder::emitAddss(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF3, 0x0F, 0x58, dst, src);
}

void InstructionEncoder::emitAddsd(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF2, 0x0F, 0x58, dst, src);
}

void InstructionEncoder::emitSubss(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF3, 0x0F, 0x5C, dst, src);
}

void InstructionEncoder::emitSubsd(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF2, 0x0F, 0x5C, dst, src);
}

void InstructionEncoder::emitMulss(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF3, 0x0F, 0x59, dst, src);
}

void InstructionEncoder::emitMulsd(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF2, 0x0F, 0x59, dst, src);
}

void InstructionEncoder::emitDivss(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF3, 0x0F, 0x5E, dst, src);
}

void InstructionEncoder::emitDivsd(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF2, 0x0F, 0x5E, dst, src);
}

void InstructionEncoder::emitMovssRR(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF3, 0x0F, 0x10, dst, src);
}

void InstructionEncoder::emitMovsdRR(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0xF2, 0x0F, 0x10, dst, src);
}

void InstructionEncoder::emitUcomiss(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0, 0x0F, 0x2E, dst, src);
}

void InstructionEncoder::emitUcomisd(std::vector<uint8_t> &out, Reg dst, Reg src)
{
    emitSseOpRR(out, 0x66, 0x0F, 0x2E, dst, src);
}

void InstructionEncoder::emitCvtsi2ss(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize)
{
    out.push_back(0xF3);
    RexPrefix rex;
    rex.w = (srcSize == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0x0F);
    out.push_back(0x2A);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitCvtsi2sd(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize)
{
    out.push_back(0xF2);
    RexPrefix rex;
    rex.w = (srcSize == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0x0F);
    out.push_back(0x2A);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitCvttss2si(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t dstSize)
{
    out.push_back(0xF3);
    RexPrefix rex;
    rex.w = (dstSize == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0x0F);
    out.push_back(0x2C);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitCvttsd2si(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t dstSize)
{
    out.push_back(0xF2);
    RexPrefix rex;
    rex.w = (dstSize == 8);
    rex.r = isExtendedReg(dst);
    rex.b = isExtendedReg(src);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }
    out.push_back(0x0F);
    out.push_back(0x2C);
    out.push_back(encodeModRM(3, getLow3Bits(dst), getLow3Bits(src)));
}

void InstructionEncoder::emitSyscall(std::vector<uint8_t> &out)
{
    out.push_back(0x0F);
    out.push_back(0x05);
}

} // namespace EzCodeEmitter::X86_64
