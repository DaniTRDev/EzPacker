#ifndef EZDSL_INST_DEF_LANG_AST_H
#define EZDSL_INST_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"
#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <string>

namespace DSL::Ast::InstDef
{

struct BitSlice
{
    uint16_t m_low{ 0 };
    uint16_t m_high{ 0 };
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

using BitExprValues =
        std::variant<Common::Identifier, Common::IntegerLiteral, SlicedIdentifier, std::shared_ptr<BitExpression>>;

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

struct InstRegOperand
{
    Common::Identifier m_regClass;
    Common::Identifier m_argName;
    InstOperandDir m_argDir{ InstOperandDir::ArgIn };
};

struct InstImmOperand
{
    Common::Identifier m_typeName;
    Common::Identifier m_argName;
    InstOperandDir m_argDir{ InstOperandDir::ArgIn };
};

using InstArgValues = std::variant<InstRegOperand, InstImmOperand>;

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
    std::pmr::vector<InstArgValues> m_args;
    Common::Identifier m_formatName;
};

using InstBodyItem = std::variant<std::pmr::vector<InstArgValues>, // IMPLICIT(...)
                                  std::pmr::vector<BitExprAssign>, // FORMAT(...)
                                  Common::StringLiteral,           // ASM(...)
                                  uint32_t,                        // LATENCY(...)
                                  std::pmr::vector<InstFlag>       // FLAGS(...)
                                  >;

struct InstBody
{
    std::pmr::vector<InstArgValues> m_implicitArgs;
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

#endif // EZDSL_INST_DEF_LANG_AST_H