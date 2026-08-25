#ifndef EZDSL_INSTRUCTION_DEF_LANG_AST_H
#define EZDSL_INSTRUCTION_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace DSL::Ast::InstDef
{

/**
 * Represents a bit-range slice (e.g., "[31:25]", "[0:7]").
 * Preserves syntax slice directionality (MSB to LSB or LSB to MSB).
 */
struct BitSlice
{
    uint16_t m_from{ 0 };
    uint16_t m_to{ 0 };
};

/**
 * AST node for an identifier sliced by bit indices (e.g. ident[0:31]).
 */
struct SlicedIdentifier
{
    Common::Identifier m_name;
    BitSlice m_slice;
};

struct BitExpression;

/**
 * Bitwise and arithmetic operators available within bitfield expressions.
 */
enum class BitExprOp
{
    Add,
    Sub,
    And,
    Or,
    Xor,
    Shl,
    Shr,
    Not
};

/**
 * Variant representing possible operand types in a bitfield expression.
 */
using BitExprValues = std::variant<Common::Identifier, Common::IntegerLiteral, SlicedIdentifier, BitExpression *>;

/**
 * Binary or unary bit expression operating on identifiers, literals, slices, or sub-expressions.
 */
struct BitExpression
{
    BitExprValues m_lhs;
    BitExprOp m_op;
    std::optional<BitExprValues> m_rhs;
};

/**
 * Field assignment in instruction encoding formats (lhs[slice] = rhs_expr).
 */
struct BitExprAssign
{
    Common::Identifier m_lhs;
    std::optional<BitSlice> m_lhsSlice;
    BitExprValues m_rhs;
};

/**
 * Single named bitfield within an instruction encoding format layout.
 */
struct FormatField
{
    Common::Identifier m_name;
    BitSlice m_slice;
    std::optional<BitExprValues> m_defaultValue;
};

/**
 * Instruction encoding format declaration (e.g., R-type, I-type) with width and bitfields.
 */
struct InstFormatDecl
{
    Common::Identifier m_name;
    uint32_t m_bitWidth{ 32 };
    std::pmr::vector<FormatField> m_fields;
};

/**
 * Dataflow direction of an instruction operand.
 */
enum class InstOperandDir : uint8_t
{
    ArgIn = 1,
    ArgOut = (1 << 1),
    ArgInOut = ArgIn | ArgOut
};

/**
 * Kind of operand in an instruction declaration (Register or Immediate).
 */
enum class InstOperandKind : uint8_t
{
    Register, // Hardware or virtual register (e.g., "GPR:rd OUT")
    Immediate // Immediate value (e.g., "simm(i12):imm12 IN", "imm(i32):val IN")
};

/**
 * Unified representation for all instruction arguments (registers & immediates).
 *
 * Examples:
 * - "GPR:rd OUT"         -> m_kind=Register,  m_typeOrClass="GPR",  m_typeParam=nullopt, m_name="rd", m_dir=ArgOut
 * - "simm(i12):imm12 IN" -> m_kind=Immediate, m_typeOrClass="simm", m_typeParam="i12",   m_name="imm12", m_dir=ArgIn
 */
struct InstOperand
{
    InstOperandKind m_kind{ InstOperandKind::Register };
    Common::Identifier m_typeOrClass;              // Register class ("GPR") or immediate ("imm", "simm", "uimm")
    std::optional<Common::Identifier> m_typeParam; // Width or type specifier (e.g., "i12", "i32")
    Common::Identifier m_name;                     // Operand identifier (e.g., "rd", "rs1", "imm12")
    InstOperandDir m_dir{ InstOperandDir::ArgIn }; // Dataflow direction ("IN", "OUT", "INOUT")
};

/**
 * Behavioral flags for target instruction definitions.
 */
enum class InstFlag : uint8_t
{
    IsBranch = 1,
    IsCall = (1 << 1),
    IsReturn = (1 << 2),
    IsTerminator = (1 << 3),
    MayLoad = (1 << 4),
    MayStore = (1 << 5),
    IsCommutative = (1 << 6),
    HasSideEffects = (1 << 7)
};

/**
 * Header declaration of a target instruction (name, formal arguments, and binary format).
 */
struct InstHeader
{
    Common::Identifier m_name;
    std::pmr::vector<InstOperand> m_args;
    Common::Identifier m_formatName;
};

using InstBodyItem = std::variant<std::pmr::vector<InstOperand>,   // IMPLICIT(...)
                                  std::pmr::vector<BitExprAssign>, // FORMAT(...)
                                  Common::StringLiteral,           // ASM(...)
                                  uint32_t,                        // LATENCY(...)
                                  std::pmr::vector<InstFlag>       // FLAGS(...)
                                  >;

/**
 * Body definition of an instruction specifying implicit operands, field assignments, assembly syntax, and flags.
 */
struct InstBody
{
    std::pmr::vector<InstOperand> m_implicitArgs;
    std::pmr::vector<BitExprAssign> m_assigns;
    std::pmr::string m_asmTemplate;
    uint32_t m_latency{ 1 };
    std::pmr::vector<InstFlag> m_flags;
};

/**
 * Full instruction declaration AST node combining header and body.
 */
struct InstDecl
{
    InstHeader m_header;
    InstBody m_body;
};

/**
 * Root AST structure representing a parsed .idf (Instruction Definition File).
 */
struct InstDefFile
{
    std::pmr::vector<InstFormatDecl> m_formats;
    std::pmr::vector<InstDecl> m_instructions;
};

} // namespace DSL::Ast::InstDef

#endif // EZDSL_INSTRUCTION_DEF_LANG_AST_H