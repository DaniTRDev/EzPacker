#ifndef EZDSL_LEGALIZE_RULE_DEF_LANG_H
#define EZDSL_LEGALIZE_RULE_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "EzDslCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::LegalizeRuleDef
{
namespace dsl = ::lexy::dsl;

/**
 * Parses an SSA variable name beginning with '$' (e.g., "$dst", "$src1").
 */
struct SsaVarName
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::lit_c<'$'> >> dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
};

/**
 * Parses predicate arguments:
 *   - SSA variables: "$offset", "$c"
 *   - Integer literals: -2048, 2047, 0xFF, 0b1010
 *   - Feature identifiers: "Zba", "HasAVX2"
 */
struct PredicateArg
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []()
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

/**
 * Parses a semantic predicate guard inside a `when` block (e.g., "immInRange($offset, -2048, 2047);",
 * "hasFeature(Zba);").
 */
struct RulePredicate
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> +
            dsl::parenthesized.list(dsl::p<PredicateArg>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>;

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg>> >>
            lexy::callback<Ast::LegalizeRuleDef::RulePredicate>(
                                          [](Ast::Common::Identifier predName,
                                             std::pmr::vector<Ast::LegalizeRuleDef::PredicateArg> args)
                                          {
                                              Ast::LegalizeRuleDef::RulePredicate pred;
                                              pred.m_predicateName = std::move(predName);
                                              pred.m_arguments = std::move(args);
                                              return pred;
                                          });
};

/**
 * Parses a compile-time transform call (e.g., "log2($shift)", "sub($amt)").
 */
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
                                              Ast::LegalizeRuleDef::RuleOperand op;
                                              op.m_kind = Ast::LegalizeRuleDef::OperandKind::CustomTransform;
                                              op.m_name = std::move(funcName);
                                              op.m_callArgs = std::move(args);
                                              return op;
                                          });
};

/**
 * Parses typed prefix operands:
 * - "i32:$dst"      (Register: type="i32", typeParam=nullopt)
 * - "imm:$c"        (Immediate: type="imm", typeParam=nullopt)
 * - "imm(i32):$c"   (Immediate: type="imm", typeParam="i32")
 * - "simm(12):$c"   (Immediate: type="simm", typeParam="12")
 */
struct TypedPrefixSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = []()
    {
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        return dsl::p<Common::Identifier> + dsl::opt(typeParam) + dsl::lit_c<':'> + dsl::p<SsaVarName>;
    }();

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::Identifier type, Ast::Common::Identifier typeParam, Ast::Common::Identifier name)
            {
                Ast::LegalizeRuleDef::RuleOperand op;
                const bool isImm = (type.m_node == "imm" || type.m_node == "simm" || type.m_node == "uimm");
                op.m_kind = isImm ? Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol
                                  : Ast::LegalizeRuleDef::OperandKind::SsaRegister;
                op.m_name = std::move(name);
                op.m_type = std::move(type);
                op.m_typeParam = std::move(typeParam);
                return op;
            },
            [](Ast::Common::Identifier type, lexy::nullopt, Ast::Common::Identifier name)
            {
                Ast::LegalizeRuleDef::RuleOperand op;
                const bool isImm = (type.m_node == "imm" || type.m_node == "simm" || type.m_node == "uimm");
                op.m_kind = isImm ? Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol
                                  : Ast::LegalizeRuleDef::OperandKind::SsaRegister;
                op.m_name = std::move(name);
                op.m_type = std::move(type);
                return op;
            });
};

/**
 * Parses ONLY bare SSA registers (e.g., "$src", "$dst").
 */
struct BareSsaOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<SsaVarName>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::Identifier name)
            {
                Ast::LegalizeRuleDef::RuleOperand op;
                op.m_kind = Ast::LegalizeRuleDef::OperandKind::SsaRegister;
                op.m_name = std::move(name);
                return op;
            });
};

/**
 * Parses a concrete integer literal immediate (e.g., 0, 42, 0xFF, -10).
 */
struct LiteralOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::IntegerLiteral>;

    static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleOperand>(
            [](Ast::Common::IntegerLiteral lit)
            {
                Ast::LegalizeRuleDef::RuleOperand op;
                op.m_kind = Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral;
                op.m_immLiteral = lit;
                return op;
            });
};

/**
 * Dispatches and constructs any instruction operand variant.
 */
struct RuleOperand
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []()
    {
        auto ident = dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore);

        // Custom transform starts with ident('$'...)
        auto customTransform = dsl::peek(ident + dsl::lit_c<'('> + dsl::lit_c<'$'>) >> dsl::p<CustomTransformOperand>;

        // Typed prefixes: "i32:$dst", "imm:$c", or "imm(i32):$c"
        auto typeParam = dsl::parenthesized(dsl::p<Common::Identifier>);
        auto typedPrefix = dsl::peek(ident + dsl::opt(typeParam) + dsl::lit_c<':'>) >> dsl::p<TypedPrefixSsaOperand>;

        // Bare SSA variable: "$dst", "$src"
        auto dollarVar = dsl::peek(dsl::lit_c<'$'>) >> dsl::p<BareSsaOperand>;

        // Literal constants: 42, 0xFF, -10
        auto literal = dsl::else_ >> dsl::p<LiteralOperand>;

        return (customTransform | typedPrefix | dollarVar | literal);
    }();

    static constexpr auto value = lexy::forward<Ast::LegalizeRuleDef::RuleOperand>;
};

/**
 * Helper production to parse comma-separated instruction operands.
 */
struct InstructionOperandList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::list(dsl::p<RuleOperand>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::RuleOperand>>;
};

/**
 * Parses an IR instruction statement or a single-operand match statement:
 *   - "ADD i32:$dst, i32:$lhs, imm(i32):$c;" (Standard instruction)
 *   - "NOP;"                                  (Nullary instruction)
 *   - "GPR:$base;"                            (Single typed operand statement)
 *   - "$base;"                                (Single bare operand statement)
 */
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
                    if (op.m_type.has_value())
                    {
                        inst.m_opcode = *op.m_type;
                    }
                    else
                    {
                        inst.m_opcode = op.m_name;
                    }
                    inst.m_operands.push_back(std::move(op));
                    return inst;
                });
    };

    struct StandardInstruction
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []()
        {
            auto name = dsl::p<Common::Identifier>;
            auto operands = dsl::opt(dsl::peek_not(dsl::lit_c<';'>) >> dsl::p<InstructionOperandList>);
            auto semicolon = dsl::lit_c<';'>;
            return name + operands + semicolon;
        }();

        static constexpr auto value = lexy::callback<Ast::LegalizeRuleDef::RuleInstruction>(
                [](Ast::Common::Identifier opcode, std::pmr::vector<Ast::LegalizeRuleDef::RuleOperand> operands)
                {
                    Ast::LegalizeRuleDef::RuleInstruction inst;
                    inst.m_opcode = std::move(opcode);
                    inst.m_operands = std::move(operands);
                    return inst;
                },
                [](Ast::Common::Identifier opcode, lexy::nullopt)
                {
                    Ast::LegalizeRuleDef::RuleInstruction inst;
                    inst.m_opcode = std::move(opcode);
                    return inst;
                });
    };

    static constexpr auto rule = []()
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

/**
 * Dispatches to match, when, or expand blocks based on the leading keyword.
 */
struct RuleBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = (Common::Keyword<"match">::rule >> dsl::p<MatchBlockBody>) |
            (Common::Keyword<"when">::rule >> dsl::p<WhenBlockBody>) |
            (Common::Keyword<"expand">::rule >> dsl::p<ExpandBlockBody>);

    static constexpr auto value = lexy::construct<RuleBlockClause>;
};

/**
 * Parses a complete rewrite rule definition with match, when, and expand blocks.
 */
struct LegalizeRewriteRule
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"rule">::rule >>
            dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<RuleBlock> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::as_list<std::vector<RuleBlockClause>> >>
            lexy::callback<Ast::LegalizeRuleDef::LegalizeRewriteRule>(
                                          [](Ast::Common::Identifier name, std::vector<RuleBlockClause> clauses)
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
                                                              {
                                                                  rule.m_matchPatterns = std::move(val.instructions);
                                                              }
                                                              else if constexpr (std::is_same_v<T, WhenClause>)
                                                              {
                                                                  rule.m_predicates = std::move(val.predicates);
                                                              }
                                                              else if constexpr (std::is_same_v<T, ExpandClause>)
                                                              {
                                                                  rule.m_expansionSequence =
                                                                          std::move(val.instructions);
                                                              }
                                                          },
                                                          clause);
                                              }
                                              return rule;
                                          });
};

/**
 * Root parser for the full rewrite rule translation unit.
 */
struct TargetLegalizeRuleDef
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<LegalizeRewriteRule> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::as_list<std::pmr::vector<Ast::LegalizeRuleDef::LegalizeRewriteRule>> >>
            lexy::callback<Ast::LegalizeRuleDef::TargetLegalizeRuleDef>(
                                          [](std::pmr::vector<Ast::LegalizeRuleDef::LegalizeRewriteRule> rules)
                                          {
                                              Ast::LegalizeRuleDef::TargetLegalizeRuleDef target;
                                              target.m_rules = std::move(rules);
                                              return target;
                                          });
};

} // namespace DSL::Parser::LegalizeRuleDef

#endif // EZDSL_LEGALIZE_RULE_DEF_LANG_H