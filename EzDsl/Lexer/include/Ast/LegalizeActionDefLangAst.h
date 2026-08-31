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
    Custom,       // Delegate legalization to a target-specific C++ callback.
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
    std::optional<std::pmr::vector<Common::Identifier>> m_customRules;
};

struct LegalizeInstructionDecl
{
    Common::Identifier m_instName;
    std::pmr::vector<LegalizeActionClause> m_actionClauses;
};

struct LegalizeActionFile
{
    std::pmr::vector<LegalizeInstructionDecl> m_legalizeInstrDecls;
};
}; // namespace DSL::Ast::LegalizeActionDef

#endif // EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H
