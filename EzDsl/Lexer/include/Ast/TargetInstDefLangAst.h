#ifndef EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
#define EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"
#include "EncodingDefLangAst.h"
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
    std::optional<Encoding::EncodingDecl> m_encoding;    // optional generic ENCODING block
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
