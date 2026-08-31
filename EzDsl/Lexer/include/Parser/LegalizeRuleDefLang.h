#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::LegalizeRuleDef
{
namespace dsl = ::lexy::dsl;

struct SsaVarName
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::lit_c<'$'> >> dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

struct PredicateArg
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (dsl::peek(dsl::lit_c<'$'>) >> dsl::p<SsaVarName>) |
            (dsl::peek(dsl::ascii::digit | dsl::lit_c<'-'> | dsl::lit_c<'+'>) >> dsl::p<Common::IntegerLiteral>) |
            (dsl::else_ >> dsl::p<Common::Identifier>);

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::PredicateArg>(
            [](Ast::Common::Identifier id) { return Ast::LegalizeRuleDef::PredicateArg{ std::move(id) }; },
            [](Ast::Common::IntegerLiteral lit) { return Ast::LegalizeRuleDef::PredicateArg{ lit }; });
};

struct RuleWhen
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ArgList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<PredicateArg>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg>>;
    };

    static constexpr auto rule = []
    {
        auto name = dsl::p<Common::Identifier>;
        auto args = dsl::parenthesized(dsl::opt(
                dsl::peek(dsl::ascii::alpha_digit_underscore | dsl::lit_c<'$'> | dsl::lit_c<'-'> | dsl::lit_c<'+'>) >>
                dsl::p<ArgList>));
        return name + args + dsl::lit_c<';'>;
    }();

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleWhen>(
            [](Ast::Common::Identifier name, std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg> args) {
                return Ast::LegalizeRuleDef::RuleWhen{ .m_predicateName = std::move(name),
                                                       .m_arguments = std::move(args) };
            },
            [](Ast::Common::Identifier name, lexy::nullopt)
            { return Ast::LegalizeRuleDef::RuleWhen{ .m_predicateName = std::move(name), .m_arguments = {} }; });
};

struct CustomTransformOperand
{
    static constexpr auto whitespace = Common::Whitespace;

    struct VarList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<SsaVarName>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::parenthesized(dsl::p<VarList>);

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstructionOperand>(
            [](Ast::Common::Identifier funcName, std::pmr::vector<Ast::Common::Identifier> args)
            {
                Ast::LegalizeRuleDef::RuleInstructionOperand op{};
                op.m_kind = Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform;
                op.m_name = std::move(funcName);
                op.m_callArgs = std::move(args);
                return op;
            });
};

struct TypedPrefixSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = []
    {
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        return dsl::p<Common::Identifier> + dsl::opt(typeParam) + dsl::lit_c<':'> + dsl::p<SsaVarName>;
    }();

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstructionOperand>(
            [](Ast::Common::Identifier type, Ast::Common::Identifier typeParam, Ast::Common::Identifier name)
            {
                const bool isImm = type.m_node == "imm";
                return Ast::LegalizeRuleDef::RuleInstructionOperand{
                    .m_kind = isImm ? Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol
                                    : Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister,
                    .m_name = std::move(name),
                    .m_type = std::move(type),
                    .m_typeParam = std::move(typeParam)
                };
            },
            [](Ast::Common::Identifier type, lexy::nullopt, Ast::Common::Identifier name)
            {
                const bool isImm = type.m_node == "imm";
                return Ast::LegalizeRuleDef::RuleInstructionOperand{
                    .m_kind = isImm ? Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol
                                    : Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister,
                    .m_name = std::move(name),
                    .m_type = std::move(type),
                    .m_typeParam = std::nullopt
                };
            });
};

struct BareSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<SsaVarName>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstructionOperand>(
            [](Ast::Common::Identifier name)
            {
                return Ast::LegalizeRuleDef::RuleInstructionOperand{
                    .m_kind = Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister,
                    .m_name = std::move(name)
                };
            });
};

struct LiteralOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::IntegerLiteral>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstructionOperand>(
            [](Ast::Common::IntegerLiteral lit)
            {
                return Ast::LegalizeRuleDef::RuleInstructionOperand{
                    .m_kind = Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral,
                    .m_immLiteral = lit
                };
            });
};

struct RuleOperand
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);
        auto customTransform = dsl::peek(ident + dsl::lit_c<'('> + dsl::lit_c<'$'>) >> dsl::p<CustomTransformOperand>;
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto typedPrefix = dsl::peek(ident + dsl::opt(typeParam) + dsl::lit_c<':'>) >> dsl::p<TypedPrefixSsaOperand>;
        auto dollarVar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<BareSsaOperand>;
        auto literal = dsl::else_ >> dsl::p<LiteralOperand>;

        return customTransform | typedPrefix | dollarVar | literal;
    }();

    static constexpr auto value = lexy::forward<Ast::LegalizeRuleDef::RuleInstructionOperand>;
};

struct InstructionOperandList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::list(dsl::p<RuleOperand>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstructionOperand>>;
};

struct RuleInstruction
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SingleOperandStatement
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<RuleOperand> + dsl::lit_c<';'>;
        static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstruction>(
                [](Ast::LegalizeRuleDef::RuleInstructionOperand op)
                {
                    Ast::LegalizeRuleDef::RuleInstruction inst;
                    inst.m_opcode = op.m_type.has_value() ? *op.m_type : op.m_name;
                    inst.m_operands.push_back(std::move(op));
                    return inst;
                });
    };

    struct StandardInstruction
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto name = dsl::p<Common::Identifier>;
            auto operands = dsl::opt(dsl::peek_not(dsl::lit_c<';'>) >> dsl::p<InstructionOperandList>);
            auto semicolon = dsl::lit_c<';'>;
            return name + operands + semicolon;
        }();

        static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstruction>(
                [](Ast::Common::Identifier opcode,
                   std::pmr::vector<Ast::LegalizeRuleDef::RuleInstructionOperand> operands) {
                    return Ast::LegalizeRuleDef::RuleInstruction{ .m_opcode = std::move(opcode),
                                                                  .m_operands = std::move(operands) };
                },
                [](Ast::Common::Identifier opcode, lexy::nullopt)
                { return Ast::LegalizeRuleDef::RuleInstruction{ .m_opcode = std::move(opcode), .m_operands = {} }; });
    };

    static constexpr auto rule = []
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);
        auto bareDollar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<SingleOperandStatement>;
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto typedSingle = dsl::peek(ident + dsl::opt(typeParam) + dsl::lit_c<':'>) >> dsl::p<SingleOperandStatement>;
        auto standard = dsl::else_ >> dsl::p<StandardInstruction>;

        return bareDollar | typedSingle | standard;
    }();

    static constexpr auto value = lexy::forward<Ast::LegalizeRuleDef::RuleInstruction>;
};

struct MatchClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction> instructions;
};

struct WhenClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleWhen> clauses;
};

struct EmitClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction> instructions;
};

using RuleBlockClause = std::variant<MatchClause, WhenClause, EmitClause>;

struct MatchBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RuleInstruction>);
    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<MatchClause>;
};

struct WhenBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RuleWhen>);
    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::RuleWhen>> >> lexy::construct<WhenClause>;
};

struct EmitBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RuleInstruction>);
    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<EmitClause>;
};

struct RuleBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (Common::Keyword<"match">::rule >> dsl::p<MatchBlockBody>) |
            (Common::Keyword<"when">::rule >> dsl::p<WhenBlockBody>) |
            ((Common::Keyword<"emit">::rule | Common::Keyword<"expand">::rule) >> dsl::p<EmitBlockBody>);

    static constexpr auto value = lexy::construct<RuleBlockClause>;
};

struct LegalizeRule
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"rule">::rule >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<RuleBlock> + dsl::lit_c<';'>));

    static constexpr auto value = Common::PmrAsList<std::pmr::vector<RuleBlockClause>> >>
            lexy::callback<Ast::LegalizeRuleDef::LegalizeRule>(
                                          [](Ast::Common::Identifier name, std::pmr::vector<RuleBlockClause> clauses)
                                          {
                                              Ast::LegalizeRuleDef::LegalizeRule rule;
                                              rule.m_ruleName = std::move(name);

                                              for (auto &clause : clauses)
                                              {
                                                  std::visit(
                                                          [&](auto &&val)
                                                          {
                                                              using T = std::decay_t<decltype(val)>;
                                                              if constexpr (std::is_same_v<T, MatchClause>)
                                                                  rule.m_matchClauses = std::move(val.instructions);
                                                              else if constexpr (std::is_same_v<T, WhenClause>)
                                                                  rule.m_whenClauses = std::move(val.clauses);
                                                              else if constexpr (std::is_same_v<T, EmitClause>)
                                                                  rule.m_emitClauses = std::move(val.instructions);
                                                          },
                                                          clause);
                                              }
                                              return rule;
                                          });
};

struct LegalizeRuleFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<LegalizeRule> + dsl::lit_c<';'>);

    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::LegalizeRuleDef::LegalizeRule>> >>
            lexy::construct<Ast::LegalizeRuleDef::LegalizeRuleFile>;
};

} // namespace DSL::Parser::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_H