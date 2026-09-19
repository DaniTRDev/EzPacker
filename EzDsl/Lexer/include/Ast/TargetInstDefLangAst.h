#ifndef EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
#define EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"
#include <optional>

namespace DSL::Ast::TargetInstDef
{

/**
 * Dataflow direction of a target instruction operand.
 */
enum class OperandDirection : uint8_t
{
    In,   // Read by the instruction.
    Out,  // Written by the instruction.
    InOut // Read and written.
};

/**
 * One operand of a target machine instruction declaration.
 */
struct TargetOperandDecl
{
    Common::Identifier m_regClassOrType;                  // e.g. "GPR32", "i32imm", "Mem32"
    Common::Identifier m_name;                            // e.g. "dst", "src"
    OperandDirection m_direction{ OperandDirection::In }; // Dataflow direction.
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
    Common::Identifier m_name;               // Declared operand name being bound.
    EncSlotKind m_slot{ EncSlotKind::None }; // Encoding slot the operand occupies.
};

/**
 * Declarative description of an instruction's machine encoding.
 */
struct EncodingDecl
{
    EncForm m_form{ EncForm::None };                 // Selected encoding shape.
    std::pmr::vector<uint8_t> m_opcode;              // 1..3 bytes
    std::optional<uint8_t> m_opcodeDigit;            // /0../7
    bool m_rexW{ false };                            // Always set REX.W for 64-bit forms.
    bool m_rexWBySize{ false };                      // set REX.W only when the size operand is 64-bit
    uint8_t m_prefixes{ 0 };                         // bitmask (see runtime EncPrefix*)
    std::pmr::vector<EncOperandBinding> m_operands;  // Operand-to-slot bindings.
    std::optional<Common::Identifier> m_coalesce;    // two-address source operand
    std::optional<Common::Identifier> m_sizeOperand; // operand determining operation size
    bool m_shiftByCL{ false };                       // Shift amount is implicitly CL.
    bool m_byteRex{ false };                         // Emit a REX prefix even for byte forms needing it.
    std::optional<uint8_t> m_condCode;               // Jcc/Setcc condition digit
    bool m_hasSseVariant{ false };                   // Encoding also carries an SSE opcode form.
    uint8_t m_ssePrefixes{ 0 };                      // Prefix bitmask for the SSE variant.
    std::pmr::vector<uint8_t> m_sseOpcode;           // Opcode bytes for the SSE variant.
};

/**
 * A target machine instruction declaration: opcode name, operand signature, optional mnemonic,
 * implicit register effects, behavioral flags, and an optional encoding block.
 */
struct TargetInstDecl
{
    Common::Identifier m_instName;                       // e.g. "ADD32rr"
    std::pmr::vector<TargetOperandDecl> m_operands;      // Typed operand signature.
    std::optional<Common::StringLiteral> m_mnemonic;     // e.g. "addl"
    std::pmr::vector<Common::Identifier> m_implicitDefs; // e.g. ["EFLAGS"]
    std::pmr::vector<Common::Identifier> m_implicitUses; // e.g. ["EAX", "EDX"]
    std::pmr::vector<Common::Identifier> m_flags;        // e.g. ["IsCommutative"]
    std::optional<EncodingDecl> m_encoding;              // optional ENCODING block
};

/**
 * Root AST node for a parsed `.idf` target instruction definition file.
 */
struct TargetInstFile
{
    std::optional<Common::Identifier> m_targetName;  // Optional target name header.
    std::pmr::vector<TargetInstDecl> m_instructions; // All declared machine instructions.
};

} // namespace DSL::Ast::TargetInstDef

#endif // EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
