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

struct PatternOperand
{
    enum class Kind
    {
        SsaRegister,       // e.g. i32:$dst
        ImmediateSymbol,   // e.g. imm(i32):$imm
        ImmediateLiteral,  // e.g. 42
        AddrModeRef,       // e.g. AddrModeRegImm($base, $disp)
        NestedTree         // e.g. (LOAD i32:$tmp, ...)
    };

    Kind m_kind{ Kind::SsaRegister };
    Common::Identifier m_name;
    std::optional<Common::Identifier> m_type;
    std::optional<Common::IntegerLiteral> m_literal;
    std::shared_ptr<PatternTree> m_nestedTree;
    std::pmr::vector<Common::Identifier> m_addrModeArgs;
};

struct PatternTree
{
    Common::Identifier m_opcode; // e.g. ADD, LOAD, BR_COND
    std::pmr::vector<PatternOperand> m_operands;
};

struct PatternWhen
{
    Common::Identifier m_predicate; // e.g. isSimm32, hasOneUse, noInterveningStore
    std::pmr::vector<Common::Identifier> m_args;
};

struct TargetEmitOperand
{
    enum class Kind
    {
        BoundVar,     // e.g. $dst, $imm
        ClassBoundVar,// e.g. GPR32:$dst
        AddrModeMem,  // e.g. [$base, $disp]
        ImmLiteral,   // e.g. 4
        PhysReg       // e.g. RAX
    };

    Kind m_kind{ Kind::BoundVar };
    Common::Identifier m_name;
    std::optional<Common::Identifier> m_regClass;
    std::optional<Common::IntegerLiteral> m_literal;
    std::pmr::vector<Common::Identifier> m_memOperands; // base, disp, index, scale
};

struct TargetEmitInst
{
    Common::Identifier m_targetOpcode;
    std::pmr::vector<TargetEmitOperand> m_operands;
};

struct SelectionPattern
{
    Common::Identifier m_name;
    uint32_t m_cost{ 1 };
    PatternTree m_matchTree;
    std::pmr::vector<PatternWhen> m_whenClauses;
    std::pmr::vector<TargetEmitInst> m_selectClauses;
};

struct AddrModeVariant
{
    Common::Identifier m_variantName;
    PatternTree m_matchTree;
    std::pmr::vector<PatternWhen> m_whenClauses;
};

struct AddrModeParam
{
    Common::Identifier m_typeOrClass;
    Common::Identifier m_name;
    std::optional<Common::IntegerLiteral> m_defaultVal;
};

struct AddrModeDecl
{
    Common::Identifier m_modeName;
    std::pmr::vector<AddrModeParam> m_params;
    std::pmr::vector<AddrModeVariant> m_variants;
};

struct InstructionSelectFile
{
    std::optional<Common::Identifier> m_targetName;
    std::pmr::vector<AddrModeDecl> m_addressingModes;
    std::pmr::vector<SelectionPattern> m_patterns;
};

} // namespace DSL::Ast::InstructionSelectDef

#endif // EZDSLLEXER_INSTRUCTION_SELECT_DEF_LANG_AST_H
