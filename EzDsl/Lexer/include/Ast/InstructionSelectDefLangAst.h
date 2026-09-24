#ifndef EZDSLLEXER_INSTRUCTION_SELECT_DEF_LANG_AST_H
#define EZDSLLEXER_INSTRUCTION_SELECT_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"
#include <memory>
#include <optional>
#include <variant>

namespace DSL::Ast::InstructionSelectDef
{

struct PatternTree;

/**
 * One operand slot within a source-pattern match tree; the variant kind determines which of
 * the optional payload fields is meaningful.
 */
struct PatternOperand
{
    enum class Kind
    {
        SsaRegister,      // e.g. i32:$dst
        ImmediateSymbol,  // e.g. imm(i32):$imm
        ImmediateLiteral, // e.g. 42
        AddrModeRef,      // e.g. AddrModeRegImm($base, $disp)
        NestedTree        // e.g. (LOAD i32:$tmp, ...)
    };

    Kind m_kind{ Kind::SsaRegister };                    // Discriminates the operand form.
    Common::Identifier m_name;                           // Bound variable or referenced mode/opcode name.
    std::optional<Common::Identifier> m_type;            // Optional type/class prefix on the operand.
    std::optional<Common::IntegerLiteral> m_literal;     // Literal value for ImmediateLiteral.
    std::shared_ptr<PatternTree> m_nestedTree;           // Sub-tree for NestedTree operands.
    std::pmr::vector<Common::Identifier> m_addrModeArgs; // Bound variables passed to an AddrModeRef.
};

/**
 * An instruction match tree: an opcode together with its pattern operands, recursively
 * describing the shape of IR the pattern expects.
 */
struct PatternTree
{
    Common::Identifier m_opcode;                 // e.g. ADD, LOAD, BR_COND
    std::pmr::vector<PatternOperand> m_operands; // Operand shapes of this node.
};

/**
 * A guard predicate attached to a pattern's `when` clause, e.g. `isSimm32($imm)`.
 */
struct PatternWhen
{
    Common::Identifier m_predicate;              // e.g. isSimm32, hasOneUse, noInterveningStore
    std::pmr::vector<Common::Identifier> m_args; // Bound variables the predicate inspects.
};

/**
 * One operand of a target instruction emitted by a `select` clause. The kind selects which
 * payload field carries the operand's meaning.
 */
struct TargetEmitOperand
{
    enum class Kind
    {
        BoundVar,      // e.g. $dst, $imm
        ClassBoundVar, // e.g. GPR32:$dst
        AddrModeMem,   // e.g. [$base, $disp]
        ImmLiteral,    // e.g. 4
        PhysReg        // e.g. RAX
    };

    Kind m_kind{ Kind::BoundVar };                      // Discriminates the emitted operand form.
    Common::Identifier m_name;                          // Variable, register, or memory base name.
    std::optional<Common::Identifier> m_regClass;       // Register class for ClassBoundVar.
    std::optional<Common::IntegerLiteral> m_literal;    // Value for ImmLiteral.
    std::pmr::vector<Common::Identifier> m_memOperands; // base, disp, index, scale
};

/**
 * A concrete target instruction to emit when a selection pattern matches, with its operands.
 */
struct TargetEmitInst
{
    Common::Identifier m_targetOpcode;              // Target instruction/opcode to produce.
    std::pmr::vector<TargetEmitOperand> m_operands; // Operands bound from the matched pattern.
};

/**
 * A complete instruction selection pattern: match tree, optional cost for pattern ranking,
 * `when` guards, and the sequence of target instructions emitted on a match.
 */
struct SelectionPattern
{
    Common::Identifier m_name;                        // Pattern name (also its symbol).
    uint32_t m_cost{ 1 };                             // Relative selection cost; lower wins.
    PatternTree m_matchTree;                          // IR shape to match.
    std::pmr::vector<PatternWhen> m_whenClauses;      // Guards that must hold for the match.
    std::pmr::vector<TargetEmitInst> m_selectClauses; // Instructions emitted on a match.
};

/**
 * One specialization of an addressing mode: the match tree that recognizes it plus optional
 * `when` guards.
 */
struct AddrModeVariant
{
    Common::Identifier m_variantName;            // Variant label.
    PatternTree m_matchTree;                     // Shape that selects this variant.
    std::pmr::vector<PatternWhen> m_whenClauses; // Guards required for the variant.
};

/**
 * A formal parameter of an addressing mode, optionally with a default integer value used when
 * the argument is omitted at a use site.
 */
struct AddrModeParam
{
    Common::Identifier m_typeOrClass;                   // Expected type or register class.
    Common::Identifier m_name;                          // Parameter name.
    std::optional<Common::IntegerLiteral> m_defaultVal; // Default value if not supplied.
};

/**
 * Declares an addressing mode: its name, formal parameters, and the set of match variants that
 * map matched IR shapes onto concrete address computations.
 */
struct AddrModeDecl
{
    Common::Identifier m_modeName;                // Mode name (also its symbol).
    std::pmr::vector<AddrModeParam> m_params;     // Declared formal parameters.
    std::pmr::vector<AddrModeVariant> m_variants; // Recognized address forms.
};

/**
 * Root AST node for a parsed `.isf` instruction selection file.
 */
struct InstructionSelectFile
{
    std::optional<Common::Identifier> m_targetName;   // Optional target name header.
    std::pmr::vector<AddrModeDecl> m_addressingModes; // All declared addressing modes.
    std::pmr::vector<SelectionPattern> m_patterns;    // All declared selection patterns.
};

} // namespace DSL::Ast::InstructionSelectDef

#endif // EZDSLLEXER_INSTRUCTION_SELECT_DEF_LANG_AST_H
