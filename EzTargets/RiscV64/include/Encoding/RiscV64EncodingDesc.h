#ifndef EZTARGETS_RISCV64_ENCODING_DESC_H
#define EZTARGETS_RISCV64_ENCODING_DESC_H

#include <cstddef>
#include <cstdint>

namespace EzTargets::RiscV64
{

/**
 * Structural role an operand plays inside a RISC-V instruction encoding.
 */
enum class EncSlotKind : uint8_t
{
    None = 0,
    Rd,          ///< Destination register (bits 11:7).
    Rs1,         ///< Source register 1 (bits 19:15).
    Rs2,         ///< Source register 2 (bits 24:20).
    Rs3,         ///< Source register 3 (bits 31:27, FMA).
    Imm12Signed, ///< I-type 12-bit signed immediate (bits 31:20).
    Imm12Store,  ///< S-type 12-bit store immediate (split bits 31:25 and 11:7).
    Imm12Branch, ///< B-type 12-bit branch immediate (split bits 31:25 and 11:7).
    Imm20Upper,  ///< U-type 20-bit upper immediate (bits 31:12).
    Imm20Jump,   ///< J-type 20-bit jump immediate (split bits 31:12).
    Shamt5,      ///< 5-bit shift amount (bits 24:20).
    Shamt6,      ///< 6-bit shift amount (bits 25:20).
    Rel,         ///< PC-relative target label / relocation.
    Csr,         ///< 12-bit CSR index (bits 31:20).
};

/**
 * Standard RISC-V instruction format shapes.
 */
enum class EncForm : uint8_t
{
    None = 0,
    RType,  ///< Register-Register (rd, rs1, rs2, funct3, funct7, opcode).
    IType,  ///< Immediate / Load / JALR (rd, rs1, imm12, funct3, opcode).
    SType,  ///< Store (rs1, rs2, imm12, funct3, opcode).
    BType,  ///< Conditional branch (rs1, rs2, imm12, funct3, opcode).
    UType,  ///< Upper immediate: LUI, AUIPC (rd, imm20, opcode).
    JType,  ///< Unconditional jump: JAL (rd, imm20, opcode).
    Pseudo, ///< Synthetic pseudo-instruction expanded at emission time.
};

/**
 * Register family expected in an operand slot.
 */
enum class EncRegClass : uint8_t
{
    GPR = 0, ///< General-purpose integer register (x0..x31).
    FPR = 1, ///< Floating-point register (f0..f31).
    VR  = 2, ///< Vector register (v0..v31).
    Any = 3, ///< Accepts any register file.
};

/**
 * Binds a declared instruction operand to a RISC-V encoding slot.
 */
struct EncOperandBinding
{
    EncSlotKind m_slot{ EncSlotKind::None };
    uint8_t m_operandIndex{ 0 };
    EncRegClass m_regClass{ EncRegClass::GPR };
};

/**
 * Declarative description of how one RISC-V target instruction is encoded.
 */
struct EncodingDesc
{
    EncForm m_form{ EncForm::None };
    uint8_t m_opcode{ 0 };  ///< 7-bit primary opcode (bits 6:0).
    uint8_t m_funct3{ 0 };  ///< 3-bit funct3 (bits 14:12).
    uint8_t m_funct7{ 0 };  ///< 7-bit funct7 (bits 31:25).
    uint8_t m_operandCount{ 0 };
    EncOperandBinding m_operands[4]{};
    uint8_t m_relocOperand{ 0xFF }; ///< Operand index carrying a PC-relative symbol reference or displacement.
};

/**
 * Memory addressing descriptor for RISC-V base + 12-bit signed displacement.
 */
struct EncMemory
{
    uint8_t m_base{ 0xFF };     ///< Hardware encoding of base register (0..31), 0xFF = none.
    int64_t m_disp{ 0 };        ///< Signed 12-bit constant displacement.
    bool m_needsReloc{ false }; ///< True when the displacement references a symbol requiring relocation.
};

/**
 * Fully resolved instruction operand for the RISC-V encoder.
 */
struct ResolvedOperand
{
    enum class Kind : uint8_t
    {
        None = 0,
        Register,
        Immediate,
        Memory,
    };

    Kind m_kind{ Kind::None };
    uint8_t m_reg{ 0 };
    bool m_isFpr{ false };
    bool m_isVr{ false };
    uint8_t m_sizeBytes{ 8 };
    int64_t m_imm{ 0 };
    EncMemory m_mem{};
    bool m_needsReloc{ false };
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_ENCODING_DESC_H
