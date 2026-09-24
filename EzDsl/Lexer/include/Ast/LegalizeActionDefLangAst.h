#ifndef EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H
#define EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "Ast/CommonAstNodes.h"

namespace DSL::Ast::LegalizeActionDef
{
/**
 * Legalization action applied to an instruction/type combination.
 */
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

/**
 * Constrains a legalization clause to a type, optionally at a specific operand index.
 */
struct TypeConstraint
{
    Common::Identifier m_type;                            // Required type name.
    std::optional<Common::IntegerLiteral> m_operandIndex; // Operand slot the constraint applies to.
};

/**
 * A single legalization action applied to an instruction/type combination, carrying whichever
 * target payload is meaningful for its kind (target type, libcall, lowering handler, or rules).
 */
struct LegalizeActionClause
{
    LegalizeActionKind m_kind;                                         // Action kind (LEGAL, WIDENS, ...).
    std::pmr::vector<TypeConstraint> m_types;                          // Input type constraints.
    std::optional<Common::Identifier> m_targetType;                    // Target type for WIDENS/NARROWS/BITCAST.
    std::optional<Common::StringLiteral> m_libcallSymbol;              // Runtime symbol for LIBCALL.
    std::optional<Common::Identifier> m_lowerHandler;                  // Handler name for LOWER.
    std::optional<std::pmr::vector<Common::Identifier>> m_customRules; // Rules invoked by CUSTOM.
};

/**
 * CLAMP_SCALAR(min, max) macro expansion input: scalar types below min widen to it, types above
 * max narrow to it, and anything in between is legal.
 */
struct ClampScalarClause
{
    Common::Identifier m_minType; // Smallest legal scalar type.
    Common::Identifier m_maxType; // Largest legal scalar type.
};

/**
 * A named, reusable set of types referenced by type constraints.
 */
struct TypeSetDecl
{
    Common::Identifier m_name;                    // Type set name.
    std::pmr::vector<Common::Identifier> m_types; // Member type names.
};

/**
 * A `group` declaration applying one set of legalization actions to several opcodes at once.
 */
struct InstructionGroupDecl
{
    Common::Identifier m_groupName;                         // Group name.
    std::pmr::vector<Common::Identifier> m_instructions;    // Opcodes the actions apply to.
    std::pmr::vector<LegalizeActionClause> m_actionClauses; // Shared action clauses.
    std::optional<ClampScalarClause> m_clampClause;         // Optional clamp macro expansion.
};

/**
 * A direct `action` declaration binding legalization actions to a single opcode.
 */
struct LegalizeInstructionDecl
{
    Common::Identifier m_instName;                          // Target opcode.
    std::pmr::vector<LegalizeActionClause> m_actionClauses; // Declared action clauses.
    std::optional<ClampScalarClause> m_clampClause;         // Optional clamp macro expansion.
};

/**
 * Root AST node for a parsed `.lad` legalization action file.
 */
struct LegalizeActionFile
{
    std::optional<Common::Identifier> m_targetName;                 // Optional target name header.
    std::pmr::vector<TypeSetDecl> m_typeSets;                       // Reusable type sets.
    std::pmr::vector<InstructionGroupDecl> m_groups;                // Grouped action declarations.
    std::pmr::vector<LegalizeInstructionDecl> m_legalizeInstrDecls; // Per-opcode action declarations.
};
}; // namespace DSL::Ast::LegalizeActionDef

#endif // EZDSLLEXER_LEGALIZE_ACTION_DEF_LANG_AST_H
