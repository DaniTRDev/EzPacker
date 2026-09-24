#ifndef EZDSL_PARSER_INSTRUCTION_SELECT_DEF_LANG_H
#define EZDSL_PARSER_INSTRUCTION_SELECT_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/InstructionSelectDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::InstructionSelectDef
{
namespace dsl = ::lexy::dsl;

/**
 * Parses a `$name` SSA variable reference and forwards the underlying Identifier.
 */
struct SsaVarName
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::lit_c<'$'> >> dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

/**
 * Parses an addressing-mode use `Mode($a, $b)` inside a pattern and produces an AddrModeRef operand.
 */
struct AddrModeCallOperand
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses a `,`-separated list of SSA variable arguments.
     */
    struct VarList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<SsaVarName>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::parenthesized(dsl::p<VarList>);

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternOperand>(
            [](Ast::Common::Identifier modeName, std::pmr::vector<Ast::Common::Identifier> args)
            {
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = Ast::InstructionSelectDef::PatternOperand::Kind::AddrModeRef;
                op.m_name = std::move(modeName);
                op.m_addrModeArgs = std::move(args);
                return op;
            });
};

/**
 * Parses a type-prefixed operand `type(param)?:$var` and classifies it as an SSA register or
 * immediate symbol.
 */
struct TypedPrefixSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = []
    {
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        return dsl::p<Common::Identifier> + dsl::opt(typeParam) + dsl::lit_c<':'> + dsl::p<SsaVarName>;
    }();

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternOperand>(
            [](Ast::Common::Identifier type, Ast::Common::Identifier typeParam, Ast::Common::Identifier name)
            {
                const bool isImm = (type.m_node == "imm");
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = isImm ? Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateSymbol
                                  : Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister;
                op.m_name = std::move(name);
                op.m_type = std::move(typeParam); // e.g. imm(i32) -> type is i32
                return op;
            },
            [](Ast::Common::Identifier type, lexy::nullopt, Ast::Common::Identifier name)
            {
                const bool isImm = (type.m_node == "imm");
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = isImm ? Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateSymbol
                                  : Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister;
                op.m_name = std::move(name);
                op.m_type = std::move(type);
                return op;
            });
};

/**
 * Parses a bare `$var` pattern operand and produces an SSA register operand.
 */
struct BareSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<SsaVarName>;

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternOperand>(
            [](Ast::Common::Identifier name)
            {
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister;
                op.m_name = std::move(name);
                return op;
            });
};

/**
 * Parses a bare integer literal pattern operand and produces an ImmediateLiteral operand.
 */
struct LiteralOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::IntegerLiteral>;

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternOperand>(
            [](Ast::Common::IntegerLiteral lit)
            {
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateLiteral;
                op.m_literal = lit;
                return op;
            });
};

/**
 * Forward declaration allowing NestedTreeOperand to recurse into the pattern-tree rule.
 */
struct PatternTreeRule;

/**
 * Parses a parenthesized nested pattern tree `(OP ...)` and wraps it as a NestedTree operand.
 */
struct NestedTreeOperand
{
    static constexpr auto rule = dsl::parenthesized(dsl::recurse<PatternTreeRule>);

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternOperand>(
            [](Ast::InstructionSelectDef::PatternTree tree)
            {
                Ast::InstructionSelectDef::PatternOperand op;
                op.m_kind = Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree;
                op.m_name = tree.m_opcode;
                op.m_nestedTree = std::make_shared<Ast::InstructionSelectDef::PatternTree>(std::move(tree));
                return op;
            });
};

/**
 * Dispatches a pattern operand to the nested-tree, address-mode, typed-prefix, bare-SSA, or literal sub-rule.
 */
struct PatternOperandParser
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);
        auto nestedTree = dsl::peek(dsl::lit_c<'('>) >> dsl::p<NestedTreeOperand>;
        auto customAddrMode = dsl::peek(ident + dsl::lit_c<'('> + dsl::lit_c<'$'>) >> dsl::p<AddrModeCallOperand>;
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto typedPrefix = dsl::peek(ident + dsl::opt(typeParam) + dsl::lit_c<':'>) >> dsl::p<TypedPrefixSsaOperand>;
        auto dollarVar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<BareSsaOperand>;
        auto literal = dsl::else_ >> dsl::p<LiteralOperand>;

        return nestedTree | customAddrMode | typedPrefix | dollarVar | literal;
    }();

    static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::PatternOperand>;
};

/**
 * Parses a `,`-separated list of pattern operands into a PMR vector.
 */
struct PatternOperandList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::list(dsl::p<PatternOperandParser>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::PatternOperand>>;
};

/**
 * Parses an opcode plus operand list (or a single-operand shorthand) into a PatternTree.
 */
struct PatternTreeRule
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses a single-operand shorthand and derives the opcode from the operand type/name.
     */
    struct SingleOperandStmt
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<PatternOperandParser>;
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternTree>(
                [](Ast::InstructionSelectDef::PatternOperand op)
                {
                    Ast::InstructionSelectDef::PatternTree tree;
                    tree.m_opcode = op.m_type.has_value() ? *op.m_type : op.m_name;
                    tree.m_operands.push_back(std::move(op));
                    return tree;
                });
    };

    /**
     * Parses a normal `OPCODE operand, ...` tree node with an optional operand list.
     */
    struct StandardTree
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto opcode = dsl::p<Common::Identifier>;
            auto operands = dsl::opt(dsl::peek_not(dsl::lit_c<';'> | dsl::lit_c<')'> | dsl::lit_c<'}'>) >>
                                     dsl::p<PatternOperandList>);
            return opcode + operands;
        }();

        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternTree>(
                [](Ast::Common::Identifier opcode, std::pmr::vector<Ast::InstructionSelectDef::PatternOperand> ops) {
                    return Ast::InstructionSelectDef::PatternTree{ .m_opcode = std::move(opcode),
                                                                   .m_operands = std::move(ops) };
                },
                [](Ast::Common::Identifier opcode, lexy::nullopt)
                { return Ast::InstructionSelectDef::PatternTree{ .m_opcode = std::move(opcode), .m_operands = {} }; });
    };

    static constexpr auto rule = []
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);
        auto dollarVar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<SingleOperandStmt>;
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto typedSingle = dsl::peek(ident + dsl::opt(typeParam) + dsl::lit_c<':'>) >> dsl::p<SingleOperandStmt>;
        auto standard = dsl::else_ >> dsl::p<StandardTree>;

        return dollarVar | typedSingle | standard;
    }();

    static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::PatternTree>;
};

/**
 * Parses a `predicate($a, $b);` or `hasExtension("avx");` guard clause into a PatternWhen.
 */
struct PatternWhenRule
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses an argument to a when-predicate: SSA variable ($var), string literal ("avx"), or identifier (avx).
     */
    struct WhenArg
    {
        static constexpr auto rule = (dsl::peek(dsl::lit_c<'$'>) >> dsl::p<SsaVarName>) |
                (dsl::peek(dsl::lit_c<'"'>) >> dsl::p<Common::StringLiteral>) |
                (dsl::else_ >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::Common::Identifier>(
                [](Ast::Common::Identifier id) { return id; },
                [](Ast::Common::StringLiteral str)
                { return Ast::Common::Identifier{ str.m_node, str.m_sourceRef }; });
    };

    /**
     * Parses a `,`-separated list of predicate arguments.
     */
    struct ArgList
    {
        static constexpr auto rule = dsl::list(dsl::p<WhenArg>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = []
    {
        auto pred = dsl::p<Common::Identifier>;
        auto args = dsl::parenthesized(
                dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore | dsl::lit_c<'$'> | dsl::lit_c<'"'>) >>
                         dsl::p<ArgList>));
        return pred + args + dsl::lit_c<';'>;
    }();

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternWhen>(
            [](Ast::Common::Identifier pred, std::pmr::vector<Ast::Common::Identifier> args) {
                return Ast::InstructionSelectDef::PatternWhen{ .m_predicate = std::move(pred),
                                                               .m_args = std::move(args) };
            },
            [](Ast::Common::Identifier pred, lexy::nullopt)
            { return Ast::InstructionSelectDef::PatternWhen{ .m_predicate = std::move(pred), .m_args = {} }; });
};

/**
 * Dispatches an operand of a selected target instruction to the memory, class-bound, bound,
 * literal, or physical-register sub-rule.
 */
struct TargetEmitOperandRule
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses a `,`-separated list of SSA variables inside a memory operand.
     */
    struct MemList
    {
        static constexpr auto rule = dsl::list(dsl::p<SsaVarName>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    /**
     * Parses a `[$base, $disp, ...]` memory operand.
     */
    struct MemOperand
    {
        static constexpr auto rule = dsl::square_bracketed(dsl::p<MemList>);
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitOperand>(
                [](std::pmr::vector<Ast::Common::Identifier> memOps)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::AddrModeMem;
                    op.m_memOperands = std::move(memOps);
                    return op;
                });
    };

    /**
     * Parses a register-class-bound variable `RegClass:$var`.
     */
    struct ClassBoundOperand
    {
        static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<SsaVarName>;
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitOperand>(
                [](Ast::Common::Identifier regClass, Ast::Common::Identifier name)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::ClassBoundVar;
                    op.m_name = std::move(name);
                    op.m_regClass = std::move(regClass);
                    return op;
                });
    };

    /**
     * Parses a plain bound variable `$var`.
     */
    struct BoundOperand
    {
        static constexpr auto rule = dsl::p<SsaVarName>;
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitOperand>(
                [](Ast::Common::Identifier name)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::BoundVar;
                    op.m_name = std::move(name);
                    return op;
                });
    };

    /**
     * Parses an integer literal emitted operand.
     */
    struct LiteralEmitOperand
    {
        static constexpr auto rule = dsl::p<Common::IntegerLiteral>;
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitOperand>(
                [](Ast::Common::IntegerLiteral lit)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::ImmLiteral;
                    op.m_literal = lit;
                    return op;
                });
    };

    /**
     * Parses a physical register reference with an optional `%` sigil.
     */
    struct PhysRegOperand
    {
        static constexpr auto rule = dsl::opt(dsl::lit_c<'%'>) + dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitOperand>(
                [](auto, Ast::Common::Identifier name)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::PhysReg;
                    op.m_name = std::move(name);
                    return op;
                },
                [](Ast::Common::Identifier name)
                {
                    Ast::InstructionSelectDef::TargetEmitOperand op;
                    op.m_kind = Ast::InstructionSelectDef::TargetEmitOperand::Kind::PhysReg;
                    op.m_name = std::move(name);
                    return op;
                });
    };

    static constexpr auto rule = []
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);
        auto mem = dsl::peek(dsl::lit_c<'['>) >> dsl::p<MemOperand>;
        auto classBound = dsl::peek(ident + dsl::lit_c<':'>) >> dsl::p<ClassBoundOperand>;
        auto bound = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<BoundOperand>;
        auto lit = dsl::peek(dsl::ascii::digit | dsl::lit_c<'-'>) >> dsl::p<LiteralEmitOperand>;
        auto phys = dsl::else_ >> dsl::p<PhysRegOperand>;

        return mem | classBound | bound | lit | phys;
    }();

    static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::TargetEmitOperand>;
};

/**
 * Parses a `,`-separated list of target emit operands into a PMR vector.
 */
struct TargetEmitOperandList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::list(dsl::p<TargetEmitOperandRule>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::TargetEmitOperand>>;
};

/**
 * Parses a `TARGET_OPCODE operands;` instruction to emit into a TargetEmitInst.
 */
struct TargetEmitInstRule
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto opcode = dsl::p<Common::Identifier>;
        auto operands = dsl::opt(dsl::peek_not(dsl::lit_c<';'>) >> dsl::p<TargetEmitOperandList>);
        return opcode + operands + dsl::lit_c<';'>;
    }();

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::TargetEmitInst>(
            [](Ast::Common::Identifier opcode, std::pmr::vector<Ast::InstructionSelectDef::TargetEmitOperand> ops)
            {
                return Ast::InstructionSelectDef::TargetEmitInst{ .m_targetOpcode = std::move(opcode),
                                                                  .m_operands = std::move(ops) };
            },
            [](Ast::Common::Identifier opcode, lexy::nullopt) {
                return Ast::InstructionSelectDef::TargetEmitInst{ .m_targetOpcode = std::move(opcode),
                                                                  .m_operands = {} };
            });
};

/**
 * Parses a `pattern NAME [cost = N] { match {...} when {...} select {...} };` declaration into a
 * SelectionPattern.
 */
struct SelectionPatternRule
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the `match { TREE; }` block containing the single match tree.
     */
    struct MatchBlock
    {
        static constexpr auto rule = Common::Keyword<"match">::rule >>
                (dsl::curly_bracketed(dsl::p<PatternTreeRule> + dsl::lit_c<';'>) + dsl::opt(dsl::lit_c<';'>));
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternTree>(
                [](Ast::InstructionSelectDef::PatternTree tree, auto...) { return tree; });
    };

    /**
     * Recognizes a single guard clause inside the when block.
     */
    struct WhenItem
    {
        static constexpr auto rule = dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<PatternWhenRule>;
        static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::PatternWhen>;
    };

    /**
     * Parses the optional `when { ... }` guard block into a PMR vector.
     */
    struct WhenBlock
    {
        static constexpr auto rule = Common::Keyword<"when">::rule >>
                (dsl::curly_bracketed.list(dsl::p<WhenItem>) + dsl::opt(dsl::lit_c<';'>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>> >>
                lexy::callback<std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>>(
                                              [](std::pmr::vector<Ast::InstructionSelectDef::PatternWhen> whens,
                                                 auto...) { return whens; });
    };

    /**
     * Recognizes a single target instruction inside the select block.
     */
    struct SelectItem
    {
        static constexpr auto rule = dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<TargetEmitInstRule>;
        static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::TargetEmitInst>;
    };

    /**
     * Parses the `select { ... }` block into a PMR vector of target instructions.
     */
    struct SelectBlock
    {
        static constexpr auto rule = Common::Keyword<"select">::rule >>
                (dsl::curly_bracketed.list(dsl::p<SelectItem>) + dsl::opt(dsl::lit_c<';'>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::TargetEmitInst>> >>
                lexy::callback<std::pmr::vector<Ast::InstructionSelectDef::TargetEmitInst>>(
                                              [](std::pmr::vector<Ast::InstructionSelectDef::TargetEmitInst> insts,
                                                 auto...) { return insts; });
    };

    /**
     * Parses the optional `[cost = N]` attribute.
     */
    struct CostDecl
    {
        static constexpr auto rule = dsl::square_bracketed(Common::Keyword<"cost">::rule >> dsl::lit_c<'='> >>
                                                           dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::forward<Ast::Common::IntegerLiteral>;
    };

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        auto cost = dsl::opt(dsl::peek(dsl::lit_c<'['>) >> dsl::p<CostDecl>);
        auto when = dsl::opt(dsl::peek(Common::Keyword<"when">::rule) >> dsl::p<WhenBlock>);
        auto body = dsl::curly_bracketed(dsl::p<MatchBlock> + when + dsl::p<SelectBlock>);
        auto semi = dsl::opt(dsl::lit_c<';'>);

        return Common::Keyword<"pattern">::rule >> (name + cost + body + semi);
    }();

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::SelectionPattern>(
            [](Ast::Common::Identifier name,
               auto optCost,
               Ast::InstructionSelectDef::PatternTree matchTree,
               auto optWhen,
               std::pmr::vector<Ast::InstructionSelectDef::TargetEmitInst> selectInsts,
               auto...)
            {
                Ast::InstructionSelectDef::SelectionPattern pat;
                pat.m_name = std::move(name);
                if constexpr (std::is_same_v<std::decay_t<decltype(optCost)>, Ast::Common::IntegerLiteral>)
                {
                    pat.m_cost = static_cast<uint32_t>(optCost.m_node);
                }
                pat.m_matchTree = std::move(matchTree);
                if constexpr (std::is_same_v<std::decay_t<decltype(optWhen)>,
                                             std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>>)
                {
                    pat.m_whenClauses = std::move(optWhen);
                }
                pat.m_selectClauses = std::move(selectInsts);
                return pat;
            });
};

/**
 * Parses a `variant NAME { match {...} when {...} }` addressing-mode specialization.
 */
struct AddrModeVariantRule
{
    /**
     * Parses the `match { TREE; }` block containing the variant's match tree.
     */
    struct MatchBlock
    {
        static constexpr auto rule = Common::Keyword<"match">::rule >>
                (dsl::curly_bracketed(dsl::p<PatternTreeRule> + dsl::lit_c<';'>) + dsl::opt(dsl::lit_c<';'>));
        static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::PatternTree>(
                [](Ast::InstructionSelectDef::PatternTree tree, auto...) { return tree; });
    };

    /**
     * Recognizes a single guard clause inside the variant's when block.
     */
    struct WhenItem
    {
        static constexpr auto rule = dsl::peek(dsl::ascii::alpha_underscore) >> dsl::p<PatternWhenRule>;
        static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::PatternWhen>;
    };

    /**
     * Parses the optional `when { ... }` guard block into a PMR vector.
     */
    struct WhenBlock
    {
        static constexpr auto rule = Common::Keyword<"when">::rule >>
                (dsl::curly_bracketed.list(dsl::p<WhenItem>) + dsl::opt(dsl::lit_c<';'>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>> >>
                lexy::callback<std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>>(
                                              [](std::pmr::vector<Ast::InstructionSelectDef::PatternWhen> whens,
                                                 auto...) { return whens; });
    };

    static constexpr auto rule = Common::Keyword<"variant">::rule >>
            (dsl::p<Common::Identifier> +
             dsl::curly_bracketed(dsl::p<MatchBlock> +
                                  dsl::opt(dsl::peek(Common::Keyword<"when">::rule) >> dsl::p<WhenBlock>)) +
             dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::AddrModeVariant>(
            [](Ast::Common::Identifier name, Ast::InstructionSelectDef::PatternTree matchTree, auto optWhen, auto...)
            {
                Ast::InstructionSelectDef::AddrModeVariant v;
                v.m_variantName = std::move(name);
                v.m_matchTree = std::move(matchTree);
                if constexpr (std::is_same_v<std::decay_t<decltype(optWhen)>,
                                             std::pmr::vector<Ast::InstructionSelectDef::PatternWhen>>)
                {
                    v.m_whenClauses = std::move(optWhen);
                }
                return v;
            });
};

/**
 * Parses one addressing-mode parameter `type:name` with an optional `= default`.
 */
struct AddrModeParamRule
{
    static constexpr auto rule = []
    {
        auto type = dsl::p<Common::Identifier>;
        auto colon = dsl::lit_c<':'>;
        auto name = dsl::p<Common::Identifier>;
        auto defaultVal = dsl::opt(dsl::lit_c<'='> >> dsl::p<Common::IntegerLiteral>);
        return type + colon + name + defaultVal;
    }();

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::AddrModeParam>(
            [](Ast::Common::Identifier type, Ast::Common::Identifier name, Ast::Common::IntegerLiteral defVal)
            {
                return Ast::InstructionSelectDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                                 .m_name = std::move(name),
                                                                 .m_defaultVal = defVal };
            },
            [](Ast::Common::Identifier type, Ast::Common::Identifier name, lexy::nullopt)
            {
                return Ast::InstructionSelectDef::AddrModeParam{ .m_typeOrClass = std::move(type),
                                                                 .m_name = std::move(name),
                                                                 .m_defaultVal = std::nullopt };
            });
};

/**
 * Parses `addrmode NAME(params) { variant ... }` into an AddrModeDecl.
 */
struct AddrModeDeclRule
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the `,`-separated formal parameter list into a PMR vector.
     */
    struct ParamList
    {
        static constexpr auto rule = dsl::list(dsl::p<AddrModeParamRule>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::AddrModeParam>>;
    };

    /**
     * Recognizes a single `variant` declaration.
     */
    struct VariantItem
    {
        static constexpr auto rule = dsl::peek(Common::Keyword<"variant">::rule) >> dsl::p<AddrModeVariantRule>;
        static constexpr auto value = lexy::forward<Ast::InstructionSelectDef::AddrModeVariant>;
    };

    /**
     * Parses the curly-braced list of variants into a PMR vector.
     */
    struct VariantList
    {
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<VariantItem>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::InstructionSelectDef::AddrModeVariant>>;
    };

    static constexpr auto rule = Common::Keyword<"addrmode">::rule >>
            (dsl::p<Common::Identifier> + dsl::parenthesized(dsl::p<ParamList>) + dsl::p<VariantList> +
             dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::AddrModeDecl>(
            [](Ast::Common::Identifier name,
               std::pmr::vector<Ast::InstructionSelectDef::AddrModeParam> params,
               std::pmr::vector<Ast::InstructionSelectDef::AddrModeVariant> variants,
               auto...)
            {
                return Ast::InstructionSelectDef::AddrModeDecl{ .m_modeName = std::move(name),
                                                                .m_params = std::move(params),
                                                                .m_variants = std::move(variants) };
            });
};

/**
 * Parses the optional `target NAME;` header and forwards the target identifier.
 */
struct TargetHeader
{
    static constexpr auto rule = Common::Keyword<"target">::rule >> (dsl::p<Common::Identifier> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

/**
 * Dispatches a top-level `.isf` item to an addressing-mode or selection-pattern parser and
 * exposes the result as a tagged variant.
 */
struct FileItem
{
    struct TagAddrMode
    {
        Ast::InstructionSelectDef::AddrModeDecl val; // Parsed addressing mode.
    };
    struct TagPattern
    {
        Ast::InstructionSelectDef::SelectionPattern val; // Parsed selection pattern.
    };

    using ItemVariant = std::variant<TagAddrMode, TagPattern>;

    /**
     * Wraps a parsed AddrModeDecl as a TagAddrMode.
     */
    struct AddrModeItem
    {
        static constexpr auto rule = dsl::p<AddrModeDeclRule>;
        static constexpr auto value = lexy::callback<TagAddrMode>([](Ast::InstructionSelectDef::AddrModeDecl decl)
                                                                  { return TagAddrMode{ std::move(decl) }; });
    };

    /**
     * Wraps a parsed SelectionPattern as a TagPattern.
     */
    struct PatternItem
    {
        static constexpr auto rule = dsl::p<SelectionPatternRule>;
        static constexpr auto value = lexy::callback<TagPattern>([](Ast::InstructionSelectDef::SelectionPattern pat)
                                                                 { return TagPattern{ std::move(pat) }; });
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"addrmode">::rule) >> dsl::p<AddrModeItem>) |
            (dsl::peek(Common::Keyword<"pattern">::rule) >> dsl::p<PatternItem>);

    static constexpr auto value = lexy::forward<ItemVariant>;
};

/**
 * Parses a whole `.isf` file as an optional target header plus an EOF-terminated item list.
 */
struct InstructionSelectFile
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the sequence of top-level items into a PMR vector of variants.
     */
    struct ItemList
    {
        static constexpr auto rule = dsl::list(dsl::p<FileItem>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<FileItem::ItemVariant>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof)(dsl::opt(dsl::p<TargetHeader>) + dsl::p<ItemList>);

    static constexpr auto value = lexy::callback<Ast::InstructionSelectDef::InstructionSelectFile>(
            [](auto targetOpt, std::pmr::vector<FileItem::ItemVariant> items)
            {
                Ast::InstructionSelectDef::InstructionSelectFile file;
                if constexpr (std::is_same_v<std::decay_t<decltype(targetOpt)>, Ast::Common::Identifier>)
                {
                    file.m_targetName = std::move(targetOpt);
                }
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, FileItem::TagAddrMode>)
                                {
                                    file.m_addressingModes.push_back(std::move(val.val));
                                }
                                else if constexpr (std::is_same_v<T, FileItem::TagPattern>)
                                {
                                    file.m_patterns.push_back(std::move(val.val));
                                }
                            },
                            item);
                }
                return file;
            });
};

} // namespace DSL::Parser::InstructionSelectDef

#endif // EZDSL_PARSER_INSTRUCTION_SELECT_DEF_LANG_H
