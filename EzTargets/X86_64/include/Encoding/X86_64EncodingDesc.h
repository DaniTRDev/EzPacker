#ifndef EZTARGETS_X86_64_ENCODING_DESC_H
#define EZTARGETS_X86_64_ENCODING_DESC_H

#include <cstddef>
#include <cstdint>

namespace EzTargets::X86_64
{

/**
 * Standard machine condition codes used by conditional branches and conditional sets.
 *
 * The numeric values are architecture-defined digits folded into an opcode byte; the
 * x86-64 values happen to be the canonical encoding shared by Jcc and SETcc.
 */
enum class ConditionCode : uint8_t
{
    O = 0x0,  ///< Overflow.
    NO = 0x1, ///< Not overflow.
    B = 0x2,  ///< Below / carry (unsigned less-than).
    AE = 0x3, ///< Above or equal / not carry (unsigned greater-or-equal).
    E = 0x4,  ///< Equal / zero.
    NE = 0x5, ///< Not equal / not zero.
    BE = 0x6, ///< Below or equal (unsigned less-or-equal).
    A = 0x7,  ///< Above (unsigned greater-than).
    S = 0x8,  ///< Sign (negative).
    NS = 0x9, ///< Not sign (non-negative).
    P = 0xA,  ///< Parity even.
    NP = 0xB, ///< Parity odd.
    L = 0xC,  ///< Less (signed less-than).
    GE = 0xD, ///< Greater or equal (signed greater-or-equal).
    LE = 0xE, ///< Less or equal (signed less-or-equal).
    G = 0xF   ///< Greater (signed greater-than).
};

/**
 * Describes the role a given instruction operand plays inside an instruction encoding.
 *
 * The slot kind is intentionally target-agnostic: it names a structural position
 * (ModR/M.reg, ModR/M.rm, immediate, relative branch field, ...) rather than an
 * x86-64 specific concept, so new targets can reuse the same runtime encoder.
 */
enum class EncSlotKind : uint8_t
{
    None = 0,
    Reg,        ///< ModR/M.reg field (a register operand).
    RmReg,      ///< ModR/M.rm field, register-direct form.
    RmMem,      ///< ModR/M.rm field, memory form (plus optional SIB/displacement).
    Imm8,       ///< 1-byte immediate.
    Imm16,      ///< 2-byte immediate.
    Imm32,      ///< 4-byte immediate.
    Imm64,      ///< 8-byte immediate.
    Imm8Signed, ///< Sign-extended 1-byte immediate (promoted by the form when it does not fit).
    Rel8,       ///< 1-byte PC-relative placeholder.
    Rel32,      ///< 4-byte PC-relative placeholder.
    CondCode,   ///< Constant condition-code digit folded into an opcode byte.
};

/**
 * The encoding "shape" of an instruction. A form selects the structural algorithm
 * used by the runtime encoder and determines how the binding list is interpreted.
 */
enum class EncForm : uint8_t
{
    None = 0,
    Rr,      ///< opcode /r : reg, rm(reg)
    Rm,      ///< opcode /r : reg <- rm(mem)
    Mr,      ///< opcode /r : rm(mem) <- reg
    Ri,      ///< ALU-style reg, imm (0x80/0x81/0x83 selection)
    MovRI,   ///< B8+rd / C7 /0 immediate move (with 64-bit MOVABS special case)
    Movzx,   ///< 0F B6/B7 /r zero-extend
    Movsx,   ///< 0F BE/BF /r or 63 /r sign-extend
    Lea,     ///< 8D /r load effective address
    Unary,   ///< F6/F7 /digit single-register unary op
    Test,    ///< F6/F7 /0 register-register test
    Shift,   ///< D0/D1/C0/C1 immediate shift or D2/D3 CL shift
    ImulRR,  ///< 0F AF /r
    ImulRI,  ///< 69/6B /r immediate multiply
    Div,     ///< F6/F7 /6 or /7 integer divide
    Jcc,     ///< 0F 80+cc cd
    Jmp,     ///< E9 cd or FF /4 r
    Call,    ///< E8 cd or FF /2 r
    Ret,     ///< C3
    Push,    ///< 50+rd
    Pop,     ///< 58+rd
    Nop,     ///< 90
    Syscall, ///< 0F 05
    Setcc,   ///< 0F 90+cc /0
    Sse,     ///< mandatory-prefix 0F opcode /r (SSE register-register)
    Cvt,     ///< mandatory-prefix 0F 2A/2C /r scalar conversion
};

/**
 * Register family hint used when selecting an instruction's SSE variant. It does not
 * affect ModR/M/REX computation, only which of the two opcode streams is chosen.
 */
enum class EncRegClass : uint8_t
{
    GPR = 0, ///< General-purpose integer register.
    FPR = 1, ///< Floating-point/vector register (XMM).
    Any = 2, ///< Accept either register file.
};

/**
 * Binds a declared instruction operand to a structural encoding slot.
 */
struct EncOperandBinding
{
    EncSlotKind m_slot{ EncSlotKind::None };    ///< Structural slot this binding occupies.
    uint8_t m_operandIndex{ 0 };                ///< Index into the resolved operand list.
    EncRegClass m_regClass{ EncRegClass::GPR }; ///< Register file expected in the slot.
};

/**
 * Declarative description of how one target instruction is encoded into bytes.
 *
 * The structure is a plain aggregate with fixed-size storage so encoding tables can be
 * emitted as `inline constexpr` arrays without dynamic allocation or static init order
 * hazards. Fields default to "absent" semantics (0xFF sentinels, empty opcode).
 */
struct EncodingDesc
{
    EncForm m_form{ EncForm::None }; ///< Structural encoding algorithm to interpret this descriptor with.

    /// Legacy/mandatory prefix bitmask. See EncPrefix* constants below.
    uint8_t m_prefixes{ 0 };
    /// REX.W policy: 0 = never, 1 = always, 2 = set when the operand size is 8 bytes.
    uint8_t m_rexW{ 0 };
    /// Fixed ModR/M.reg digit (0..7) or 0xFF when the `Reg` slot supplies it.
    uint8_t m_digit{ 0xFF };
    /// Opcode byte sequence (1..3 bytes).
    uint8_t m_opcode[3]{ 0, 0, 0 };
    uint8_t m_opcodeLen{ 0 };
    /// Number of valid entries in m_operands.
    uint8_t m_operandCount{ 0 };
    EncOperandBinding m_operands[4]{}; ///< Operand-to-slot bindings, in declaration order.
    /// Operand index used to determine the operation size; 0xFF for "first register".
    uint8_t m_sizeOperand{ 0xFF };
    /// Operand index copied into operand 0 before encoding (two-address coalescing); 0xFF disabled.
    uint8_t m_coalesceSrc{ 0xFF };
    /// Operand index carrying a PC-relative symbol reference requiring a relocation; 0xFF disabled.
    uint8_t m_relocOperand{ 0xFF };
    /// True when the shift count comes from CL (no immediate byte).
    bool m_shiftByCL{ false };
    /// True for the MOV r64, imm64 form (encoder selects C7 /0 vs B8+rd imm64).
    bool m_movabs{ false };
    /// True when byte-register encoding must force a REX prefix (SPL/BPL/SIL/DIL).
    bool m_byteRex{ false };
    /// Constant condition code folded into the Jcc/Setcc opcode (0..15).
    uint8_t m_condCode{ 0 };
    /// Makes the encoder use the SSE variant (see below) when the register operand is an FPR.
    bool m_hasSseVariant{ false };
    /// SSE variant prefix/opcode (used when m_hasSseVariant is set and operands are FPR).
    uint8_t m_ssePrefixes{ 0 };
    uint8_t m_sseOpcode[3]{ 0, 0, 0 };
    uint8_t m_sseOpcodeLen{ 0 };
};

/// EncPrefix bitmask values used by EncodingDesc::m_prefixes / m_ssePrefixes.
inline constexpr uint8_t EncPrefix66 = 1u << 0;
inline constexpr uint8_t EncPrefix67 = 1u << 1;
inline constexpr uint8_t EncPrefixF2 = 1u << 2;
inline constexpr uint8_t EncPrefixF3 = 1u << 3;
inline constexpr uint8_t EncPrefixF0 = 1u << 4;

/**
 * Describes a memory addressing form in a target-neutral way.
 *
 * `m_base`/`m_index` hold hardware encodings (0..31 in the x86-64 convention where
 * values 16..31 identify FPR/XMM registers). 0xFF means "no base/index".
 */
struct EncMemory
{
    uint8_t m_base{ 0xFF };  ///< Hardware encoding of the base register, or 0xFF for none.
    uint8_t m_index{ 0xFF }; ///< Hardware encoding of the index register, or 0xFF for none.
    uint8_t m_scale{ 1 };    ///< Index scale factor (1, 2, 4 or 8).
    int64_t m_disp{ 0 };     ///< Constant displacement added to the effective address.
    bool m_ripRel{ false };  ///< True for RIP-relative addressing (disp32 relative to next IP).
    /// True when the displacement is a symbol reference that must be relocated.
    bool m_needsReloc{ false };
};

/**
 * A fully resolved instruction operand, decoupled from EzMir.
 *
 * The target emitter is responsible for converting EzMir operands (registers, stack
 * frame objects, globals, ...) into this representation before calling the encoder.
 */
struct ResolvedOperand
{
    enum class Kind : uint8_t
    {
        None = 0,  ///< Unset/absent operand.
        Register,  ///< Physical register operand.
        Immediate, ///< Integer immediate operand.
        Memory,    ///< Memory addressing operand.
    };

    Kind m_kind{ Kind::None }; ///< Discriminant selecting which payload below is active.
    /// Hardware encoding of a register operand (0..31; >=16 identifies an FPR/XMM).
    uint8_t m_reg{ 0 };
    bool m_isFpr{ false }; ///< True when the register belongs to the floating-point/vector file.
    /// Size of the operand in bytes (used for prefix/REX.W selection).
    uint8_t m_sizeBytes{ 8 };
    int64_t m_imm{ 0 }; ///< Immediate value for Kind::Immediate.
    EncMemory m_mem{};  ///< Addressing form for Kind::Memory.
    /// True when this operand references a symbol requiring a relocation.
    bool m_needsReloc{ false };
};

} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_ENCODING_DESC_H
