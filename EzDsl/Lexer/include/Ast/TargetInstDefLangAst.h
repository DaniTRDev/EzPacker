#ifndef EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
#define EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"
#include <optional>

namespace DSL::Ast::TargetInstDef
{

enum class OperandDirection : uint8_t
{
    In,
    Out,
    InOut
};

struct TargetOperandDecl
{
    Common::Identifier m_regClassOrType; // e.g. "GPR32", "i32imm", "Mem32"
    Common::Identifier m_name;           // e.g. "dst", "src"
    OperandDirection m_direction{ OperandDirection::In };
};

/**
 * Encoding shape selected by an `ENCODING { form: ...; }` block.
 */
enum class EncForm : uint8_t
{
    None = 0,
    Rr,
    Rm,
    Mr,
    Ri,
    MovRI,
    Movzx,
    Movsx,
    Lea,
    Unary,
    Test,
    Shift,
    ImulRR,
    ImulRI,
    Div,
    Jcc,
    Jmp,
    Call,
    Ret,
    Push,
    Pop,
    Nop,
    Syscall,
    Setcc,
    Sse,
    Cvt,
};

/**
 * Structural slot an operand name binds to within an encoding.
 */
enum class EncSlotKind : uint8_t
{
    None = 0,
    Reg,
    RmReg,
    RmMem,
    Imm8,
    Imm16,
    Imm32,
    Imm64,
    Imm8Signed,
    Rel8,
    Rel32,
    CondCode,
};

/**
 * Binds a declared operand name to an encoding slot, e.g. `src2 => reg`.
 */
struct EncOperandBinding
{
    Common::Identifier m_name;
    EncSlotKind m_slot{ EncSlotKind::None };
};

/**
 * Declarative description of an instruction's machine encoding.
 */
struct EncodingDecl
{
    EncForm m_form{ EncForm::None };
    std::pmr::vector<uint8_t> m_opcode;   // 1..3 bytes
    std::optional<uint8_t> m_opcodeDigit; // /0../7
    bool m_rexW{ false };
    bool m_rexWBySize{ false }; // set REX.W only when the size operand is 64-bit
    uint8_t m_prefixes{ 0 };    // bitmask (see runtime EncPrefix*)
    std::pmr::vector<EncOperandBinding> m_operands;
    std::optional<Common::Identifier> m_coalesce;    // two-address source operand
    std::optional<Common::Identifier> m_sizeOperand; // operand determining operation size
    bool m_shiftByCL{ false };
    bool m_byteRex{ false };
    std::optional<uint8_t> m_condCode; // Jcc/Setcc condition digit
    bool m_hasSseVariant{ false };
    uint8_t m_ssePrefixes{ 0 };
    std::pmr::vector<uint8_t> m_sseOpcode;
};

struct TargetInstDecl
{
    Common::Identifier m_instName; // e.g. "ADD32rr"
    std::pmr::vector<TargetOperandDecl> m_operands;
    std::optional<Common::StringLiteral> m_mnemonic;     // e.g. "addl"
    std::pmr::vector<Common::Identifier> m_implicitDefs; // e.g. ["EFLAGS"]
    std::pmr::vector<Common::Identifier> m_implicitUses; // e.g. ["EAX", "EDX"]
    std::pmr::vector<Common::Identifier> m_flags;        // e.g. ["IsCommutative"]
    std::optional<EncodingDecl> m_encoding;              // optional ENCODING block
};

struct TargetInstFile
{
    std::optional<Common::Identifier> m_targetName;
    std::pmr::vector<TargetInstDecl> m_instructions;
};

} // namespace DSL::Ast::TargetInstDef

#endif // EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
