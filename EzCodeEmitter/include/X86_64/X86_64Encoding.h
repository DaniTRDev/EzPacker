#ifndef EZPACKER_X86_64_ENCODING_H
#define EZPACKER_X86_64_ENCODING_H

#include "EzCodeEmitterCommon.h"
#include "X86_64Registers.h"

namespace EzCodeEmitter::X86_64
{

/**
 * x86-64 REX prefix structure (0x40 - 0x4F).
 * - W: 1 = 64-bit operand size.
 * - R: Extension of the ModR/M reg field (bit 3).
 * - X: Extension of the SIB index field (bit 3).
 * - B: Extension of the ModR/M r/m, SIB base, or opcode reg field (bit 3).
 */
struct RexPrefix
{
    bool w{ false };
    bool r{ false };
    bool x{ false };
    bool b{ false };

    constexpr bool isNeeded() const { return w || r || x || b; }

    constexpr uint8_t encode() const
    {
        return 0x40 | (w ? 0x08 : 0) | (r ? 0x04 : 0) | (x ? 0x02 : 0) | (b ? 0x01 : 0);
    }
};

/**
 * Standard x86 conditional branch condition codes (4-bit, 0x0 - 0xF).
 */
enum class ConditionCode : uint8_t
{
    O   = 0x0, // Overflow
    NO  = 0x1, // Not Overflow
    B   = 0x2, // Below / Carry (unsigned <)
    AE  = 0x3, // Above or Equal / Not Carry (unsigned >=)
    E   = 0x4, // Equal / Zero
    NE  = 0x5, // Not Equal / Not Zero
    BE  = 0x6, // Below or Equal (unsigned <=)
    A   = 0x7, // Above (unsigned >)
    S   = 0x8, // Sign (negative)
    NS  = 0x9, // Not Sign (non-negative)
    P   = 0xA, // Parity / Parity Even
    NP  = 0xB, // Not Parity / Parity Odd
    L   = 0xC, // Less (signed <)
    GE  = 0xD, // Greater or Equal (signed >=)
    LE  = 0xE, // Less or Equal (signed <=)
    G   = 0xF  // Greater (signed >)
};

/**
 * Standard ALU operation codes mapped to x86 primary opcodes and extension digits.
 */
enum class AluOp : uint8_t
{
    ADD = 0,
    OR  = 1,
    ADC = 2,
    SBB = 3,
    AND = 4,
    SUB = 5,
    XOR = 6,
    CMP = 7
};

/**
 * Comprehensive representation of x86-64 memory addressing modes:
 *  - [base]
 *  - [base + disp]
 *  - [base + index * scale + disp]
 *  - [index * scale + disp]
 *  - [rip + disp32]
 */
struct MemoryOperand
{
    enum class Kind : uint8_t
    {
        BaseDisp,
        BaseIndexScaleDisp,
        RipRel
    };

    Kind m_kind{ Kind::BaseDisp };
    Reg m_base{ Reg::None };
    Reg m_index{ Reg::None };
    uint8_t m_scale{ 1 }; // 1, 2, 4, 8
    int64_t m_disp{ 0 };

    static MemoryOperand Base(Reg base)
    {
        MemoryOperand m;
        m.m_kind = Kind::BaseDisp;
        m.m_base = base;
        m.m_disp = 0;
        return m;
    }

    static MemoryOperand BaseDisp(Reg base, int64_t disp)
    {
        MemoryOperand m;
        m.m_kind = Kind::BaseDisp;
        m.m_base = base;
        m.m_disp = disp;
        return m;
    }

    static MemoryOperand BaseIndex(Reg base, Reg index, uint8_t scale = 1, int64_t disp = 0)
    {
        MemoryOperand m;
        m.m_kind = Kind::BaseIndexScaleDisp;
        m.m_base = base;
        m.m_index = index;
        m.m_scale = scale;
        m.m_disp = disp;
        return m;
    }

    static MemoryOperand IndexDisp(Reg index, uint8_t scale, int64_t disp)
    {
        MemoryOperand m;
        m.m_kind = Kind::BaseIndexScaleDisp;
        m.m_base = Reg::None;
        m.m_index = index;
        m.m_scale = scale;
        m.m_disp = disp;
        return m;
    }

    static MemoryOperand RipRel(int32_t disp)
    {
        MemoryOperand m;
        m.m_kind = Kind::RipRel;
        m.m_disp = disp;
        return m;
    }
};

/**
 * Low-level byte-exact instruction encoder emitting raw bytes for x86-64 machine instructions.
 */
class InstructionEncoder
{
  public:
    /**
     * Encodes ModR/M byte: (mod << 6) | ((reg & 7) << 3) | (rm & 7).
     */
    static constexpr uint8_t encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm)
    {
        return ((mod & 0x03) << 6) | ((reg & 0x07) << 3) | (rm & 0x07);
    }

    /**
     * Encodes SIB byte: (scale << 6) | ((index & 7) << 3) | (base & 7).
     */
    static constexpr uint8_t encodeSIB(uint8_t scalePower, uint8_t index, uint8_t base)
    {
        return ((scalePower & 0x03) << 6) | ((index & 0x07) << 3) | (base & 0x07);
    }

    /**
     * Encodes a ModR/M and optional SIB and displacement bytes into the output vector.
     */
    static void encodeModRMSIB(std::vector<uint8_t> &out,
                               RexPrefix &rex,
                               uint8_t regOrOpcodeDigit,
                               const MemoryOperand &mem);

    // =========================================================================
    // MOV Instructions
    // =========================================================================

    /**
     * MOV r, r (sizes: 1, 2, 4, 8 bytes).
     */
    static void emitMovRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t size = 8);

    /**
     * MOV r, [m] (sizes: 1, 2, 4, 8 bytes).
     */
    static void emitMovRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t size = 8);

    /**
     * MOV [m], r (sizes: 1, 2, 4, 8 bytes).
     */
    static void emitMovMR(std::vector<uint8_t> &out, const MemoryOperand &dst, Reg src, uint8_t size = 8);

    /**
     * MOV r, imm (sizes: 1, 2, 4, 8 bytes).
     * For size=8, uses C7 /0 imm32 if imm fits in int32_t, or B8+reg imm64 (MOVABS).
     */
    static void emitMovRI(std::vector<uint8_t> &out, Reg dst, int64_t imm, uint8_t size = 8);

    /**
     * MOV [m], imm32.
     */
    static void emitMovMI(std::vector<uint8_t> &out, const MemoryOperand &dst, int32_t imm, uint8_t size = 8);

    /**
     * MOVZX r, r (zero-extend from byte or word).
     */
    static void emitMovzxRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize);

    /**
     * MOVZX r, [m] (zero-extend from byte or word).
     */
    static void emitMovzxRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t srcSize);

    /**
     * MOVSX r, r (sign-extend from byte, word, or dword MOVSXD).
     */
    static void emitMovsxRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize);

    /**
     * MOVSX r, [m] (sign-extend from byte, word, or dword MOVSXD).
     */
    static void emitMovsxRM(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t srcSize);

    // =========================================================================
    // ALU Instructions (ADD, SUB, AND, OR, XOR, CMP)
    // =========================================================================

    static void emitAluRR(std::vector<uint8_t> &out, AluOp op, Reg dst, Reg src, uint8_t size = 8);
    static void emitAluRM(std::vector<uint8_t> &out, AluOp op, Reg dst, const MemoryOperand &src, uint8_t size = 8);
    static void emitAluMR(std::vector<uint8_t> &out, AluOp op, const MemoryOperand &dst, Reg src, uint8_t size = 8);
    static void emitAluRI(std::vector<uint8_t> &out, AluOp op, Reg dst, int32_t imm, uint8_t size = 8);
    static void emitAluMI(std::vector<uint8_t> &out, AluOp op, const MemoryOperand &dst, int32_t imm, uint8_t size = 8);

    // =========================================================================
    // TEST & LEA
    // =========================================================================

    static void emitTestRR(std::vector<uint8_t> &out, Reg r1, Reg r2, uint8_t size = 8);
    static void emitTestRI(std::vector<uint8_t> &out, Reg r, int32_t imm, uint8_t size = 8);
    static void emitLea(std::vector<uint8_t> &out, Reg dst, const MemoryOperand &src, uint8_t size = 8);

    // =========================================================================
    // PUSH / POP
    // =========================================================================

    static void emitPushR(std::vector<uint8_t> &out, Reg reg);
    static void emitPushImm8(std::vector<uint8_t> &out, int8_t imm);
    static void emitPushImm32(std::vector<uint8_t> &out, int32_t imm);
    static void emitPushM(std::vector<uint8_t> &out, const MemoryOperand &mem);
    static void emitPopR(std::vector<uint8_t> &out, Reg reg);
    static void emitPopM(std::vector<uint8_t> &out, const MemoryOperand &mem);

    // =========================================================================
    // Control Flow Instructions
    // =========================================================================

    static void emitJmpShort(std::vector<uint8_t> &out, int8_t disp);
    static void emitJmpNear(std::vector<uint8_t> &out, int32_t disp);
    static void emitJmpR(std::vector<uint8_t> &out, Reg reg);
    static void emitJccShort(std::vector<uint8_t> &out, ConditionCode cc, int8_t disp);
    static void emitJccNear(std::vector<uint8_t> &out, ConditionCode cc, int32_t disp);
    static void emitCallNear(std::vector<uint8_t> &out, int32_t disp);
    static void emitCallR(std::vector<uint8_t> &out, Reg reg);
    static void emitRet(std::vector<uint8_t> &out);
    static void emitRetImm(std::vector<uint8_t> &out, uint16_t imm);
    static void emitNop(std::vector<uint8_t> &out, size_t count = 1);

    // =========================================================================
    // Shift Instructions (SHL, SHR, SAR)
    // =========================================================================
    static void emitShlRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size = 8);
    static void emitShlRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size = 8);
    static void emitShrRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size = 8);
    static void emitShrRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size = 8);
    static void emitSarRI(std::vector<uint8_t> &out, Reg dst, uint8_t amt, uint8_t size = 8);
    static void emitSarRCL(std::vector<uint8_t> &out, Reg dst, uint8_t size = 8);

    // =========================================================================
    // Unary Arithmetic (NEG, NOT)
    // =========================================================================
    static void emitNegR(std::vector<uint8_t> &out, Reg reg, uint8_t size = 8);
    static void emitNotR(std::vector<uint8_t> &out, Reg reg, uint8_t size = 8);

    // =========================================================================
    // Multiply & Divide (IMUL, IDIV, DIV)
    // =========================================================================
    static void emitImulRR(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t size = 8);
    static void emitImulRI(std::vector<uint8_t> &out, Reg dst, Reg src, int32_t imm, uint8_t size = 8);
    static void emitIdivR(std::vector<uint8_t> &out, Reg src, uint8_t size = 8);
    static void emitDivR(std::vector<uint8_t> &out, Reg src, uint8_t size = 8);

    // =========================================================================
    // Conditional Set (SETcc)
    // =========================================================================
    static void emitSetcc(std::vector<uint8_t> &out, ConditionCode cc, Reg dst);

    // =========================================================================
    // Floating Point / SSE Instructions
    // =========================================================================
    static void emitAddss(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitAddsd(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitSubss(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitSubsd(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitMulss(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitMulsd(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitDivss(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitDivsd(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitMovssRR(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitMovsdRR(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitUcomiss(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitUcomisd(std::vector<uint8_t> &out, Reg dst, Reg src);
    static void emitCvtsi2ss(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize = 4);
    static void emitCvtsi2sd(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t srcSize = 8);
    static void emitCvttss2si(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t dstSize = 4);
    static void emitCvttsd2si(std::vector<uint8_t> &out, Reg dst, Reg src, uint8_t dstSize = 8);

    // =========================================================================
    // System Instructions
    // =========================================================================
    static void emitSyscall(std::vector<uint8_t> &out);
};

} // namespace EzCodeEmitter::X86_64

#endif // EZPACKER_X86_64_ENCODING_H
