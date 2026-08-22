#ifndef EZDSL_LEGALIZE_MATRIX_DEF_LANG_H
#define EZDSL_LEGALIZE_MATRIX_DEF_LANG_H

#include "EzDslCommon.h"
#include "Ast/CommonAstNodes.h"

#include <optional>

namespace DSL::Ast::LegalizeMatrixDefLang
{
/**
 * Action to perform when an instruction matches a given type combination.
 */
enum class LegalizeActionKind
{
    Legal,        // The instruction and type combination is natively supported.
    WidenScalar,  // Promote scalar types to a larger legal scalar type (e.g., i8 -> i32).
    NarrowScalar, // Split scalar types into smaller legal scalar types (e.g., i64 -> 2x i32).
    Lower,        // Decompose the instruction into simpler generic IR instructions.
    Libcall,      // Lower the instruction into a runtime library call (e.g., __divdi3).
    Custom,       // Delegate legalization to a target-specific C++ callback.
    Bitcast,      // Reinterpret the value into a legal type of equal bit-width (e.g., i32 -> f32).
    Unsupported   // Explicitly reject the type combination as illegal.
};

/**
 * Represents a type constraint on a specific instruction operand slot.
 *
 * For homogeneous operations (e.g., ADD, SUB), where all operands share
 * the same type, the index is omitted (std::nullopt). The legalizer assumes
 * type index 0 (which applies uniformly to destination and inputs).
 *
 * For heterogeneous operations (e.g., SEXT, LOAD), where operand
 * types differ, an explicit type index specifies the exact slot being checked:
 * - Type index 0: Destination / Result / Value type
 * - Type index 1: Source / Pointer / ...
 *
 * Examples:
 * - "i32"     -> Homogeneous check on type index 0
 * - "i8:1"    -> Heterogeneous check targeting operand type index 1 (e.g., Source)
 */
struct TypeConstraint
{
    Common::Identifier m_type;                         // Type name (e.g., "i8", "i32", "p0", "v4f32").
    std::optional<Common::IntegerLiteral> m_typeIndex; // Optional operand slot index (e.g., 0, 1).
};

/**
 * A single legalization directive within an instruction declaration.
 *
 * Specifies which types trigger this action, along with optional transformation
 * targets (such as target types for widening/narrowing or symbol names for libcalls).
 *
 * Syntax Examples:
 * - LEGAL(i8, i16, i32);
 * - WIDEN(i1, i2, i4) >> i32;
 * - WIDEN(i1:1, i8:1) >> i32;
 * - NARROW(i64) >> i32;
 * - LIBCALL(i64) >> "__divdi3";
 * - LOWER(i8:1);
 */
struct LegalizeActionClause
{
    LegalizeActionKind m_kind;
    std::pmr::vector<TypeConstraint> m_types;             // Types matched by this action.
    std::optional<Common::Identifier> m_targetType;       // Target type for Widen/Narrow/Bitcast (e.g., "i32").
    std::optional<Common::StringLiteral> m_libcallSymbol; // Runtime library symbol name for Libcall.
};

/**
 * Encapsulates all legalization rules declared for a specific generic IR opcode.
 *
 * Example:
 * @code
 * action ADD {
 *     LEGAL(i32, f32);
 *     WIDEN(i1, i8, i16) >> i32;
 *     NARROW(i64) >> i32;
 * };
 * @endcode
 */
struct InstructionLegalizeDecl
{
    Common::Identifier m_instName;                    // Generic IR opcode name (e.g., "ADD", "SEXT").
    std::pmr::vector<LegalizeActionClause> m_actions; // Directives declared for this instruction.
};

/**
 * Root AST node representing a complete target legalization definition file.
 *
 * Defines the full action table mapping opcodes to their legality rules.
 */
struct TargetLegalizeDef
{
    std::pmr::vector<InstructionLegalizeDecl> m_instructionActions; // Per-instruction legalization declarations.
};
}; // namespace DSL::Ast::LegalizeMatrixDefLang

#endif // EZDSL_LEGALIZE_MATRIX_DEF_LANG_H
