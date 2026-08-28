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
 * Represents a bit-range slice within instruction bitfields.
 *
 * Syntax:
 *   BitSlice := '[' IntegerLiteral ':' IntegerLiteral ']'
 *
 * Examples:
 *   [31:0]
 *   [12:14]
 */
struct BitSlice
{
    uint16_t m_from{ 0 };
    uint16_t m_to{ 0 };
};

/**
 * AST node for an identifier indexed by a bit slice.
 *
 * Syntax:
 *   SlicedIdentifier := Identifier BitSlice
 *
 * Example:
 *   imm12[0:4]
 */
struct SlicedIdentifier
{
    Common::Identifier m_name;
    BitSlice m_slice;
};

struct BitExpression;

/**
 * Bitwise and arithmetic operators available within bitfield expressions.
 *
 * Operator tokens:
 *   Add: '+'   Sub: '-'   And: '&'   Or: '|'   Xor: '^'   Shl: '<<'   Shr: '>>'   Not: '~'
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
 * AST node representing a bit expression (unary or binary with precedence).
 *
 * Syntax:
 *   BitExpression := BitOrExpr
 *   BitOrExpr     := BitXorExpr ( '|' BitXorExpr )*
 *   BitXorExpr    := BitAndExpr ( '^' BitAndExpr )*
 *   BitAndExpr    := BitShiftExpr ( '&' BitShiftExpr )*
 *   BitShiftExpr  := BitAddExpr ( ('<<' | '>>') BitAddExpr )*
 *   BitAddExpr    := BitUnaryExpr ( ('+' | '-') BitUnaryExpr )*
 *   BitUnaryExpr  := '~'? BitAtom
 *   BitAtom       := '(' BitExpression ')' | IntegerLiteral | SlicedIdentifier | Identifier
 */
struct BitExpression
{
    BitExprValues m_lhs;
    BitExprOp m_op;
    std::optional<BitExprValues> m_rhs;
};

/**
 * Field assignment in instruction encoding formats.
 *
 * Syntax:
 *   BitExprAssign := Identifier BitSlice? '=' BitExpression
 *
 * Examples:
 *   opcode = 0x33
 *   imm4_0[0:4] = imm12[0:4]
 */
struct BitExprAssign
{
    Common::Identifier m_lhs;
    std::optional<BitSlice> m_lhsSlice;
    BitExprValues m_rhs;
};

/**
 * Single named bitfield within an instruction encoding format layout.
 *
 * Syntax:
 *   FormatField := Identifier BitSlice ( '=' BitExpression )? ';'
 *
 * Examples:
 *   opcode[0:6] = 0x33;
 *   rd[7:11];
 */
struct FormatField
{
    Common::Identifier m_name;
    BitSlice m_slice;
    std::optional<BitExprValues> m_defaultValue;
};

/**
 * Instruction encoding format declaration (e.g., R-type, I-type) with width and bitfields.
 *
 * Syntax:
 *   InstFormatDecl := 'format' FormatName ( '(' BitWidth ')' )? '{' ( FormatField )* '}' ';'?
 *   FormatName     := Identifier
 *   BitWidth       := IntegerLiteral (defaults to 32 if omitted)
 *
 * Example:
 *   format RType(32) {
 *       opcode[0:6] = 0x33;
 *       rd[7:11];
 *       funct3[12:14];
 *       rs1[15:19];
 *       rs2[20:24];
 *       funct7[25:31];
 *   };
 */
struct InstFormatDecl
{
    Common::Identifier m_name;
    uint32_t m_bitWidth{ 32 };
    std::pmr::vector<FormatField> m_fields;
};

/**
 * Dataflow direction of an instruction operand.
 *
 * Valid direction keywords:
 *   'IN', 'OUT', 'INOUT'
 */
enum class InstOperandDir : uint8_t
{
    ArgIn = 1,
    ArgOut = (1 << 1),
    ArgInOut = ArgIn | ArgOut
};

/**
 * Kind of operand in an instruction declaration.
 */
enum class InstOperandKind : uint8_t
{
    Register, // Hardware or virtual register (e.g., "GPR:rd OUT")
    Immediate // Immediate value (e.g., "simm(i12):imm12 IN", "imm(i32):val IN")
};

/**
 * Unified representation for all instruction arguments (registers & immediates).
 *
 * Syntax:
 *   InstOperand := ( RegClass | ImmType ('(' ImmWidth ')')? ) ':' OperandName Direction
 *   RegClass    := Identifier
 *   ImmType     := 'imm' | 'simm' | 'uimm'
 *   ImmWidth    := Identifier
 *   OperandName := Identifier
 *   Direction   := 'IN' | 'OUT' | 'INOUT'
 *
 * Examples:
 *   GPR:rd OUT
 *   simm(i12):imm12 IN
 *   imm:val IN
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
 *
 * Valid flag keywords:
 *   'isBranch', 'isCall', 'isReturn', 'isTerminator', 'mayLoad', 'mayStore', 'commutative', 'volatile'
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
 *
 * Syntax:
 *   InstHeader := 'inst' InstName '(' ( InstOperand (',' InstOperand)* )? ')' 'format' FormatName
 *   InstName   := Identifier
 *   FormatName := Identifier
 *
 * Example:
 *   inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType
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
 *
 * Syntax:
 *   InstBody := '{' ( BodyItem )* '}'
 *   BodyItem := ImplicitDecl | FormatDecl | AsmDecl | LatencyDecl | FlagsDecl
 *   ImplicitDecl := 'IMPLICIT' '(' ( InstOperand (',' InstOperand)* )? ')' ';'
 *   FormatDecl   := 'FORMAT' '(' ( BitExprAssign (',' BitExprAssign)* )? ')' ';'
 *   AsmDecl      := 'ASM' '(' StringLiteral ')' ';'
 *   LatencyDecl  := 'LATENCY' '(' IntegerLiteral ')' ';'
 *   FlagsDecl    := 'FLAGS' '(' ( InstFlag (',' InstFlag)* )? ')' ';'
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
 *
 * Syntax:
 *   InstDecl := InstHeader InstBody ';'?
 *
 * Example:
 *   inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType {
 *       FORMAT(funct3 = 0b000, funct7 = 0b0000000);
 *       ASM("add ${rd}, ${rs1}, ${rs2}");
 *       LATENCY(1);
 *       FLAGS(commutative);
 *   };
 */
struct InstDecl
{
    InstHeader m_header;
    InstBody m_body;
};

/**
 * Root AST structure representing a parsed .idf (Instruction Definition File).
 *
 * Syntax:
 *   InstDefFile := ( InstFormatDecl | InstDecl )* EOF
 */
struct InstDefFile
{
    std::pmr::vector<InstFormatDecl> m_formats;
    std::pmr::vector<InstDecl> m_instructions;
};

} // namespace DSL::Ast::InstDef

#endif // EZDSL_INSTRUCTION_DEF_LANG_AST_H