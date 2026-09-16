#ifndef EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H
#define EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Ast::LegalizeActionDef
{
enum class LegalizeActionKind : uint8_t
{
    Legal,        // The instruction and type combination is natively supported.
    WidenScalar,  // Promote scalar types to a larger legal scalar type (e.g., i8 -> i32).
    NarrowScalar, // Split scalar types into smaller legal scalar types (e.g., i64 -> 2x i32).
    Libcall,      // Lower the instruction into a runtime library call (e.g., __divdi3).
    Lower,        // Standard procedural lowering (CALL, RET, ALLOC).
    Custom,       // Delegate legalization to a target-specific C++ callback or rewrite rule.
    Bitcast,      // Reinterpret the value into a legal type of equal bit-width (e.g., i32 -> f32).
    Unsupported   // Explicitly reject the type combination as illegal.
};

struct TypeConstraint
{
    Common::Identifier m_type;
    std::optional<Common::IntegerLiteral> m_operandIndex;
};

struct LegalizeActionClause
{
    LegalizeActionKind m_kind;
    std::pmr::vector<TypeConstraint> m_types;
    std::optional<Common::Identifier> m_targetType;
    std::optional<Common::StringLiteral> m_libcallSymbol;
    std::optional<Common::Identifier> m_lowerHandler;
    std::optional<std::pmr::vector<Common::Identifier>> m_customRules;
};

struct ClampScalarClause
{
    Common::Identifier m_minType;
    Common::Identifier m_maxType;
};

struct TypeSetDecl
{
    Common::Identifier m_name;
    std::pmr::vector<Common::Identifier> m_types;
};

struct InstructionGroupDecl
{
    Common::Identifier m_groupName;
    std::pmr::vector<Common::Identifier> m_instructions;
    std::pmr::vector<LegalizeActionClause> m_actionClauses;
    std::optional<ClampScalarClause> m_clampClause;
};

struct LegalizeInstructionDecl
{
    Common::Identifier m_instName;
    std::pmr::vector<LegalizeActionClause> m_actionClauses;
    std::optional<ClampScalarClause> m_clampClause;
};

struct LegalizeActionFile
{
    std::optional<Common::Identifier> m_targetName;
    std::pmr::vector<TypeSetDecl> m_typeSets;
    std::pmr::vector<InstructionGroupDecl> m_groups;
    std::pmr::vector<LegalizeInstructionDecl> m_legalizeInstrDecls;
};
}; // namespace DSL::Ast::LegalizeActionDef

#endif // EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H
