#ifndef EZDSL_LEGALIZE_ACTION_DEF_LANG_AST_H
#define EZDSL_LEGALIZE_ACTION_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "Ast/CommonAstNodes.h"

#include <optional>

namespace DSL::Ast::LegalizeActionDef
{
/**
 * Action to perform when an instruction matches a given type combination.
 *
 * Valid action keywords:
 *   'LEGAL', 'WIDENS', 'NARROWS', 'LIBCALL', 'CUSTOM', 'BITCAST', 'UNSUPPORTED'
 */
enum class LegalizeActionKind : uint8_t
{
    Legal,        // The instruction and type combination is natively supported.
    WidenScalar,  // Promote scalar types to a larger legal scalar type (e.g., i8 -> i32).
    NarrowScalar, // Split scalar types into smaller legal scalar types (e.g., i64 -> 2x i32).
    Libcall,      // Lower the instruction into a runtime library call (e.g., __divdi3).
    Custom,       // Delegate legalization to a target-specific C++ callback.
    Bitcast,      // Reinterpret the value into a legal type of equal bit-width (e.g., i32 -> f32).
    Lower,        // Expand or lower the instruction using rewrite rules or standard expansion routines.
    Unsupported   // Explicitly reject the type combination as illegal.
};

/**
 * Type constraint on an instruction operand slot.
 *
 * Syntax:
 *   TypeConstraint := TypeName ( ':' OperandSlotIndex )?
 *   TypeName       := Identifier
 *   OperandSlotIndex := IntegerLiteral
 *
 * Examples:
 *   - "i32"     -> Homogeneous check on default type index 0
 *   - "i8:1"    -> Heterogeneous check targeting operand slot index 1
 */
struct TypeConstraint
{
    Common::Identifier m_type;
    std::optional<Common::IntegerLiteral> m_operandIndex;
};

/**
 * Single legalization directive within an instruction declaration.
 *
 * Syntax:
 *   LegalizeActionClause := ActionKind '(' TypeConstraint (',' TypeConstraint)* ')' ( '>>' Target )?
 *   ActionKind := 'LEGAL' | 'WIDENS' | 'NARROWS' | 'LIBCALL' | 'CUSTOM' | 'BITCAST' | 'UNSUPPORTED'
 *   Target     := Identifier | StringLiteral
 *
 * Examples:
 *   LEGAL(i8, i16, i32)
 *   WIDENS(i1, i2, i4) >> i32
 *   LIBCALL(i64) >> "__divdi3"
 */
struct LegalizeActionClause
{
    LegalizeActionKind m_kind;
    std::pmr::vector<TypeConstraint> m_types;
    std::optional<Common::Identifier> m_targetType;
    std::optional<Common::StringLiteral> m_libcallSymbol;
};

/**
 * Encapsulates all legalization rules declared for a specific generic IR opcode in a .lad file.
 *
 * Syntax:
 *   InstructionLegalizeDecl := 'action' OpcodeName '{' ( LegalizeActionClause ';' )* '}' ';'?
 *   OpcodeName              := Identifier
 *
 * Example:
 *   action ADD {
 *       LEGAL(i32, f32);
 *       WIDENS(i1, i8, i16) >> i32;
 *       NARROWS(i64) >> i32;
 *   };
 */
struct InstructionLegalizeDecl
{
    Common::Identifier m_instName;
    std::pmr::vector<LegalizeActionClause> m_actions;
};

/**
 * Root AST node representing a complete target legalization definition file (.lad).
 *
 * Syntax:
 *   TargetLegalizeDef := ( InstructionLegalizeDecl )* EOF
 */
struct TargetLegalizeDef
{
    std::pmr::vector<InstructionLegalizeDecl> m_instructionActions;
};
}; // namespace DSL::Ast::LegalizeActionDef

#endif // EZDSL_LEGALIZE_ACTION_DEF_LANG_AST_H
