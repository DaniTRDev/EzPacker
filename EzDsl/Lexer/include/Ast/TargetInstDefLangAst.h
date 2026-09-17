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
    Common::Identifier m_name;            // e.g. "dst", "src"
    OperandDirection m_direction{ OperandDirection::In };
};

struct TargetInstDecl
{
    Common::Identifier m_instName;                         // e.g. "ADD32rr"
    std::pmr::vector<TargetOperandDecl> m_operands;
    std::optional<Common::StringLiteral> m_mnemonic;       // e.g. "addl"
    std::pmr::vector<Common::Identifier> m_implicitDefs;   // e.g. ["EFLAGS"]
    std::pmr::vector<Common::Identifier> m_implicitUses;   // e.g. ["EAX", "EDX"]
    std::pmr::vector<Common::Identifier> m_flags;          // e.g. ["IsCommutative"]
};

struct TargetInstFile
{
    std::optional<Common::Identifier> m_targetName;
    std::pmr::vector<TargetInstDecl> m_instructions;
};

} // namespace DSL::Ast::TargetInstDef

#endif // EZDSLLEXER_TARGET_INST_DEF_LANG_AST_H
