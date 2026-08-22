#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslCommon.h"
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

    static constexpr auto rule = []
    {
        auto dollarVar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<SsaVarName>;
        auto intLit =
                dsl::peek(dsl::lit_c<'-'> | dsl::lit_c<'+'> | dsl::ascii::digit) >> dsl::p<Common::IntegerLiteral>;
        auto bareIdent = dsl::else_ >> dsl::p<Common::Identifier>;

        return dollarVar | intLit | bareIdent;
    }();

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::PredicateArg>(
            [](Ast::Common::Identifier ident) -> Ast::LegalizeRuleDef::PredicateArg { return ident; },
            [](Ast::Common::IntegerLiteral lit) -> Ast::LegalizeRuleDef::PredicateArg { return lit; });
};

struct RulePredicate
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> +
            dsl::parenthesized.list(dsl::p<PredicateArg>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>;

    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg>> >>
            lexy::callback<Ast::LegalizeRuleDef::RulePredicate>(
                    [](Ast::Common::Identifier predName, std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg> args)
                    {
                        return Ast::LegalizeRuleDef::RulePredicate{ .m_predicateName = std::move(predName),
                                                                    .m_arguments = std::move(args) };
                    });
};

struct CustomTransformOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::p<Common::Identifier> + dsl::parenthesized.list(dsl::p<SsaVarName>, dsl::sep(dsl::lit_c<','>));

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::Common::Identifier>> >>
            lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
                                          [](Ast::Common::Identifier funcName,
                                             std::pmr::vector<Ast::Common::Identifier> args)
                                          {
                                              return Ast::LegalizeRuleDef::RuleOperand{
                                                  .m_kind = Ast::LegalizeRuleDef::OperandKind::CustomTransform,
                                                  .m_name = std::move(funcName),
                                                  .m_callArgs = std::move(args)
                                              };
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

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::Identifier type, Ast::Common::Identifier typeParam, Ast::Common::Identifier name)
            {
                const bool isImm = (type.m_node == "imm" || type.m_node == "simm" || type.m_node == "uimm");
                return Ast::LegalizeRuleDef::RuleOperand{ .m_kind = isImm
                                                                  ? Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol
                                                                  : Ast::LegalizeRuleDef::OperandKind::SsaRegister,
                                                          .m_name = std::move(name),
                                                          .m_type = std::move(type),
                                                          .m_typeParam = std::move(typeParam) };
            },
            [](Ast::Common::Identifier type, lexy::nullopt, Ast::Common::Identifier name)
            {
                const bool isImm = (type.m_node == "imm" || type.m_node == "simm" || type.m_node == "uimm");
                return Ast::LegalizeRuleDef::RuleOperand{ .m_kind = isImm
                                                                  ? Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol
                                                                  : Ast::LegalizeRuleDef::OperandKind::SsaRegister,
                                                          .m_name = std::move(name),
                                                          .m_type = std::move(type),
                                                          .m_typeParam = std::nullopt };
            });
};

struct BareSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<SsaVarName>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::Identifier name)
            {
                return Ast::LegalizeRuleDef::RuleOperand{ .m_kind = Ast::LegalizeRuleDef::OperandKind::SsaRegister,
                                                          .m_name = std::move(name) };
            });
};

struct LiteralOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::IntegerLiteral>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::IntegerLiteral lit)
            {
                return Ast::LegalizeRuleDef::RuleOperand{ .m_kind = Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral,
                                                          .m_immLiteral = lit };
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

    static constexpr auto value = lexy::forward<Ast::LegalizeRuleDef::RuleOperand>;
};

struct InstructionOperandList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::list(dsl::p<RuleOperand>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RuleOperand>>;
};

struct RuleInstruction
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SingleOperandStatement
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<RuleOperand> + dsl::lit_c<';'>;
        static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstruction>(
                [](Ast::LegalizeRuleDef::RuleOperand op)
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
                [](Ast::Common::Identifier opcode, std::pmr::vector<Ast::LegalizeRuleDef::RuleOperand> operands)
                {
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
    std::pmr::vector<Ast::LegalizeRuleDef::RulePredicate> predicates;
};

struct ExpandClause
{
    std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction> instructions;
};

using RuleBlockClause = std::variant<MatchClause, WhenClause, ExpandClause>;

struct MatchBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RuleInstruction>);
    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<MatchClause>;
};

struct WhenBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RulePredicate>);
    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RulePredicate>> >> lexy::construct<WhenClause>;
};

struct ExpandBlockBody
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<RuleInstruction>);
    static constexpr auto value =
            lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RuleInstruction>> >> lexy::construct<ExpandClause>;
};

struct RuleBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (Common::Keyword<"match">::rule >> dsl::p<MatchBlockBody>) |
            (Common::Keyword<"when">::rule >> dsl::p<WhenBlockBody>) |
            (Common::Keyword<"expand">::rule >> dsl::p<ExpandBlockBody>);

    static constexpr auto value = lexy::construct<RuleBlockClause>;
};

struct LegalizeRewriteRule
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"rule">::rule >>
            (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<RuleBlock> + dsl::lit_c<';'>));

    static constexpr auto value = lexy::as_list<std::pmr::vector<RuleBlockClause>> >>
            lexy::callback<Ast::LegalizeRuleDef::LegalizeRewriteRule>(
                                          [](Ast::Common::Identifier name, std::pmr::vector<RuleBlockClause> clauses)
                                          {
                                              Ast::LegalizeRuleDef::LegalizeRewriteRule rule;
                                              rule.m_ruleName = std::move(name);

                                              for (auto &clause : clauses)
                                              {
                                                  std::visit(
                                                          [&](auto &&val)
                                                          {
                                                              using T = std::decay_t<decltype(val)>;
                                                              if constexpr (std::is_same_v<T, MatchClause>)
                                                                  rule.m_matchPatterns = std::move(val.instructions);
                                                              else if constexpr (std::is_same_v<T, WhenClause>)
                                                                  rule.m_predicates = std::move(val.predicates);
                                                              else if constexpr (std::is_same_v<T, ExpandClause>)
                                                                  rule.m_expansionSequence =
                                                                          std::move(val.instructions);
                                                          },
                                                          clause);
                                              }
                                              return rule;
                                          });
};

struct TargetLegalizeRuleDef
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<LegalizeRewriteRule> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::LegalizeRewriteRule>> >>
            lexy::construct<Ast::LegalizeRuleDef::TargetLegalizeRuleDef>;
};

} // namespace DSL::Parser::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_H