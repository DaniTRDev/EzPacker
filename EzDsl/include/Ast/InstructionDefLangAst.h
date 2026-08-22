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

struct SlicedIdentifier
{
    Common::Identifier m_name;
    BitSlice m_slice;
};

struct BitExpression;

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

// Replaced std::shared_ptr with std::unique_ptr for strict AST tree ownership.
using BitExprValues =
        std::variant<Common::Identifier, Common::IntegerLiteral, SlicedIdentifier, std::unique_ptr<BitExpression>>;

struct BitExpression
{
    BitExprValues m_lhs;
    BitExprOp m_op;
    std::optional<BitExprValues> m_rhs;
};

struct BitExprAssign
{
    Common::Identifier m_lhs;
    std::optional<BitSlice> m_lhsSlice;
    BitExprValues m_rhs;
};

struct FormatField
{
    Common::Identifier m_name;
    BitSlice m_slice;
    std::optional<BitExprValues> m_defaultValue;
};

struct InstFormatDecl
{
    Common::Identifier m_name;
    uint32_t m_bitWidth{ 32 };
    std::pmr::vector<FormatField> m_fields;
};

enum class InstOperandDir
{
    ArgIn,
    ArgOut,
    ArgInOut
};

enum class InstOperandKind
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

enum class InstFlag
{
    IsBranch,
    IsCall,
    IsReturn,
    IsTerminator,
    MayLoad,
    MayStore,
    IsCommutative,
    HasSideEffects
};

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

struct InstBody
{
    std::pmr::vector<InstOperand> m_implicitArgs;
    std::pmr::vector<BitExprAssign> m_assigns;
    std::pmr::string m_asmTemplate;
    uint32_t m_latency{ 1 };
    std::pmr::vector<InstFlag> m_flags;
};

struct InstDecl
{
    InstHeader m_header;
    InstBody m_body;
};

struct InstDefFile
{
    std::pmr::vector<InstFormatDecl> m_formats;
    std::pmr::vector<InstDecl> m_instructions;
};

} // namespace DSL::Ast::InstDef

#endif // EZDSL_INSTRUCTION_DEF_LANG_AST_H