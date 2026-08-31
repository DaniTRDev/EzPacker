#ifndef EZDSL_CALLING_CONV_DEF_LANG_H
#define EZDSL_CALLING_CONV_DEF_LANG_H

#include "Ast/CallingConvDefLangAst.h"
#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::CallingConvDef
{
namespace dsl = ::lexy::dsl;

struct StackGrowthRule
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::StackGrowth>
        .map(LEXY_LIT("down"), Ast::CallingConvDef::StackGrowth::Down)
        .map(LEXY_LIT("up"),   Ast::CallingConvDef::StackGrowth::Up);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackGrowth>;
};

struct StackCleanupRule
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::StackCleanup>
        .map(LEXY_LIT("caller"), Ast::CallingConvDef::StackCleanup::Caller)
        .map(LEXY_LIT("callee"), Ast::CallingConvDef::StackCleanup::Callee);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackCleanup>;
};

struct AllocPolicyRule
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::AllocPolicy>
        .map(LEXY_LIT("all_or_nothing"), Ast::CallingConvDef::AllocPolicy::AllOrNothing)
        .map(LEXY_LIT("split"),          Ast::CallingConvDef::AllocPolicy::SplitRegAndStack);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::AllocPolicy>;
};

struct RegisterListRule
{
    static constexpr auto whitespace = Common::Whitespace;

    struct RegEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::forward<Ast::Common::Identifier>;
    };

    static constexpr auto rule = dsl::square_bracketed.opt_list(dsl::p<RegEntry>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<Ast::Common::Identifier>;
};

// Parses: fallback: stack(8) or stack(size, align)
struct StackFallbackRule
{
    static constexpr auto whitespace = Common::Whitespace;

    struct StackParams
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized(dsl::p<Common::IntegerLiteral> +
                                                        dsl::opt(dsl::lit_c<','> >> dsl::p<Common::IntegerLiteral>));
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::StackFallback>(
                [](Ast::Common::IntegerLiteral slot, auto align)
                {
                    Ast::CallingConvDef::StackFallback fallback;
                    fallback.m_slotSize = std::move(slot);

                    if constexpr (std::is_same_v<std::decay_t<(decltype(&align))>, Ast::Common::IntegerLiteral>)
                    {
                        fallback.m_align = std::move(align);
                    }

                    return fallback;
                });
    };

    static constexpr auto rule = Common::Keyword<"stack">::rule >> dsl::p<StackParams>;
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackFallback>;
};

// =============================================================================
// 1. STACK BLOCK
// =============================================================================

struct StackSection
{
    static constexpr auto whitespace = Common::Whitespace;

    struct AlignDecl
    {
        static constexpr auto rule = Common::Keyword<"align">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
    };
    struct GrowthDecl
    {
        static constexpr auto rule = Common::Keyword<"growth">::rule >> (dsl::lit_c<':'> >> dsl::p<StackGrowthRule>);
    };
    struct CleanupDecl
    {
        static constexpr auto rule = Common::Keyword<"cleanup">::rule >> (dsl::lit_c<':'> >> dsl::p<StackCleanupRule>);
    };
    struct ShadowDecl
    {
        static constexpr auto rule = Common::Keyword<"shadow_space">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
    };
    struct SpDecl
    {
        static constexpr auto rule = Common::Keyword<"sp">::rule >> (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    };
    struct FpDecl
    {
        static constexpr auto rule = Common::Keyword<"fp">::rule >> (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    };

    using FieldVariant = std::variant<std::pair<Common::Keyword<"align">, Ast::Common::IntegerLiteral>,
                                      std::pair<Common::Keyword<"growth">, Ast::CallingConvDef::StackGrowth>,
                                      std::pair<Common::Keyword<"cleanup">, Ast::CallingConvDef::StackCleanup>,
                                      std::pair<Common::Keyword<"shadow_space">, Ast::Common::IntegerLiteral>,
                                      std::pair<Common::Keyword<"sp">, Ast::Common::Identifier>,
                                      std::pair<Common::Keyword<"fp">, Ast::Common::Identifier>>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto a = dsl::peek(Common::Keyword<"align">::rule) >>
                    (dsl::p<AlignDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"align">{}, v); }));
            auto g = dsl::peek(Common::Keyword<"growth">::rule) >>
                    (dsl::p<GrowthDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"growth">{}, v); }));
            auto c = dsl::peek(Common::Keyword<"cleanup">::rule) >>
                    (dsl::p<CleanupDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"cleanup">{}, v); }));
            auto s = dsl::peek(Common::Keyword<"shadow_space">::rule) >>
                    (dsl::p<ShadowDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"shadow_space">{}, v); }));
            auto sp = dsl::peek(Common::Keyword<"sp">::rule) >>
                    (dsl::p<SpDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"sp">{}, std::move(v)); }));
            auto fp = dsl::peek(Common::Keyword<"fp">::rule) >>
                    (dsl::p<FpDecl> >>
                     lexy::callback<FieldVariant>([](auto v)
                                                  { return std::make_pair(Common::Keyword<"fp">{}, std::move(v)); }));
            return a | g | c | s | sp | fp;
        }();
        static constexpr auto value = lexy::forward<FieldVariant>;
    };

    static constexpr auto rule = Common::Keyword<"stack">::rule >>
            dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::StackDef>(
            [](std::pmr::vector<FieldVariant> fields)
            {
                Ast::CallingConvDef::StackDef def{};
                for (auto &f : fields)
                {
                    std::visit(
                            [&](auto &&item)
                            {
                                using T = std::decay_t<decltype(item.first)>;
                                if constexpr (std::is_same_v<T, Common::Keyword<"align">>)
                                    def.m_alignment = item.second;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"growth">>)
                                    def.m_growth = item.second;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"cleanup">>)
                                    def.m_cleanup = item.second;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"shadow_space">>)
                                    def.m_shadowSpace = item.second;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"sp">>)
                                    def.m_stackPointer = std::move(item.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"fp">>)
                                    def.m_framePointer = std::move(item.second);
                            },
                            f);
                }
                return def;
            });
};

struct PrimitiveRuleParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"types">::rule >>
            (dsl::square_bracketed.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>)) + LEXY_LIT("=>") +
             dsl::p<Common::Identifier>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::PrimitiveRule>(
            [](std::pmr::vector<Ast::Common::Identifier> types, Ast::Common::Identifier target) {
                return Ast::CallingConvDef::PrimitiveRule{ .m_types = std::move(types),
                                                           .m_targetClass = std::move(target) };
            });
};

struct TargetClassSpec
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ByRefSpec
    {
        static constexpr auto whitespace = Common::Whitespace;
        struct OptCopy
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = Common::Keyword<"implicit_copy">::rule >>
                    (dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>);
            static constexpr auto value = lexy::forward<Ast::Common::BooleanLiteral>;
        };

        static constexpr auto rule = Common::Keyword<"by_ref">::rule >> dsl::opt(dsl::parenthesized(dsl::p<OptCopy>));
        static constexpr auto value = lexy::callback<std::pair<Ast::Common::Identifier, bool>>(
                [](Ast::Common::BooleanLiteral copy)
                { return std::make_pair(Ast::Common::Identifier{ .m_name = "by_ref" }, copy.m_value); },
                [](auto...) { return std::make_pair(Ast::Common::Identifier{ .m_name = "by_ref" }, false); });
    };

    static constexpr auto rule = []
    {
        auto byRef = dsl::peek(Common::Keyword<"by_ref">::rule) >> dsl::p<ByRefSpec>;
        auto id = dsl::else_ >>
                (dsl::p<Common::Identifier> >> lexy::callback<std::pair<Ast::Common::Identifier, bool>>(
                                                       [](Ast::Common::Identifier target)
                                                       { return std::make_pair(std::move(target), false); }));
        return byRef | id;
    }();

    static constexpr auto value = lexy::forward<std::pair<Ast::Common::Identifier, bool>>;
};

struct AggregateConditionParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SizeGt
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"size">::rule >>
                (dsl::lit_c<'>'> >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](Ast::Common::IntegerLiteral val)
                {
                    return Ast::CallingConvDef::AggregateCondition{
                        .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeGreaterThan,
                        .m_sizeLimit = val
                    };
                });
    };

    struct SizeLe
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"size">::rule >>
                (LEXY_LIT("<=") >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](Ast::Common::IntegerLiteral val)
                {
                    return Ast::CallingConvDef::AggregateCondition{
                        .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeLessThanOrEqual,
                        .m_sizeLimit = val
                    };
                });
    };

    struct SizeIn
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"size">::rule >>
                (Common::Keyword<"in">::rule >>
                 dsl::square_bracketed.list(dsl::p<Common::IntegerLiteral>, dsl::sep(dsl::lit_c<','>)));
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](std::pmr::vector<Ast::Common::IntegerLiteral> sizes)
                {
                    return Ast::CallingConvDef::AggregateCondition{
                        .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeIn,
                        .m_sizeSet = std::move(sizes)
                    };
                });
    };

    struct HfaHva
    {
        static constexpr auto whitespace = Common::Whitespace;
        struct Body
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<','> +
                    Common::Keyword<"max_elements">::rule + dsl::lit_c<':'> + dsl::p<Common::IntegerLiteral>;
            static constexpr auto value =
                    lexy::callback<std::pair<Ast::Common::Identifier, Ast::Common::IntegerLiteral>>(
                            [](Ast::Common::Identifier type, Ast::Common::IntegerLiteral maxElem)
                            { return std::make_pair(std::move(type), maxElem); });
        };

        static constexpr auto rule = (Common::Keyword<"hfa">::rule | Common::Keyword<"hva">::rule) >>
                dsl::parenthesized(dsl::p<Body>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](std::pair<Ast::Common::Identifier, Ast::Common::IntegerLiteral> params)
                {
                    return Ast::CallingConvDef::AggregateCondition{
                        .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::HomogeneousAggregate,
                        .m_homoBaseType = std::move(params.first),
                        .m_homoMaxCount = params.second
                    };
                });
    };

    struct SimpleFlags
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto nt = Common::Keyword<"non_trivial">::rule >>
                    lexy::constant(Ast::CallingConvDef::AggregateCondition::Kind::NonTrivial);
            auto un = Common::Keyword<"unaligned">::rule >>
                    lexy::constant(Ast::CallingConvDef::AggregateCondition::Kind::Unaligned);
            return nt | un;
        }();
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](Ast::CallingConvDef::AggregateCondition::Kind kind)
                { return Ast::CallingConvDef::AggregateCondition{ .m_kind = kind }; });
    };

    struct CondTerm
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sizeGt = dsl::peek(Common::Keyword<"size">::rule >> dsl::lit_c<'>'>) >> dsl::p<SizeGt>;
            auto sizeLe = dsl::peek(Common::Keyword<"size">::rule >> LEXY_LIT("<=")) >> dsl::p<SizeLe>;
            auto sizeIn = dsl::peek(Common::Keyword<"size">::rule >> Common::Keyword<"in">::rule) >> dsl::p<SizeIn>;
            auto hfa = dsl::peek(Common::Keyword<"hfa">::rule | Common::Keyword<"hva">::rule) >> dsl::p<HfaHva>;
            auto flag = dsl::else_ >> dsl::p<SimpleFlags>;
            return sizeGt | sizeLe | sizeIn | hfa | flag;
        }();
        static constexpr auto value = lexy::forward<Ast::CallingConvDef::AggregateCondition>;
    };

    // Parses: when <cond> (|| <cond>)* => <target>
    struct WhenBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"when">::rule >>
                (dsl::list(dsl::p<CondTerm>, dsl::sep(LEXY_LIT("||"))) + LEXY_LIT("=>") + dsl::p<TargetClassSpec>);

        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::CallingConvDef::AggregateCondition>>(
                [](std::pmr::vector<Ast::CallingConvDef::AggregateCondition> conds,
                   std::pair<Ast::Common::Identifier, bool> target)
                {
                    for (auto &c : conds)
                    {
                        c.m_resultClass = target.first;
                        c.m_implicitCopy = target.second;
                    }
                    return conds;
                });
    };

    struct DefaultBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"default">::rule >> (LEXY_LIT("=>") + dsl::p<TargetClassSpec>);
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::CallingConvDef::AggregateCondition>>(
                [](std::pair<Ast::Common::Identifier, bool> target)
                {
                    std::pmr::vector<Ast::CallingConvDef::AggregateCondition> conds;
                    conds.push_back(Ast::CallingConvDef::AggregateCondition{
                            .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::Default,
                            .m_resultClass = target.first,
                            .m_implicitCopy = target.second });
                    return conds;
                });
    };

    static constexpr auto rule = []
    {
        auto w = dsl::peek(Common::Keyword<"when">::rule) >> dsl::p<WhenBranch>;
        auto d = dsl::else_ >> dsl::p<DefaultBranch>;
        return w | d;
    }();
    static constexpr auto value = lexy::forward<std::pmr::vector<Ast::CallingConvDef::AggregateCondition>>;
};

struct AggregatePipelineParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SliceDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"slice">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral> >> dsl::opt(Common::Keyword<"bytes">::rule));
        static constexpr auto value = lexy::forward<Ast::Common::IntegerLiteral>;
    };

    struct PrecedenceDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"precedence">::rule >>
                (dsl::lit_c<':'> >> dsl::square_bracketed.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>)));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    struct PolicyDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"policy">::rule >> (dsl::lit_c<':'> >> dsl::p<AllocPolicyRule>);
        static constexpr auto value = lexy::forward<Ast::CallingConvDef::AllocPolicy>;
    };

    using ItemVariant =
            std::variant<std::pmr::vector<Ast::CallingConvDef::AggregateCondition>,
                         std::pair<Common::Keyword<"slice">, Ast::Common::IntegerLiteral>,
                         std::pair<Common::Keyword<"precedence">, std::pmr::vector<Ast::Common::Identifier>>,
                         std::pair<Common::Keyword<"policy">, Ast::CallingConvDef::AllocPolicy>>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto cond = dsl::peek(Common::Keyword<"when">::rule | Common::Keyword<"default">::rule) >>
                    (dsl::p<AggregateConditionParser> >>
                     lexy::callback<ItemVariant>([](auto v) { return ItemVariant{ std::move(v) }; }));
            auto slice = dsl::peek(Common::Keyword<"slice">::rule) >>
                    (dsl::p<SliceDecl> >>
                     lexy::callback<ItemVariant>([](auto v) { return std::make_pair(Common::Keyword<"slice">{}, v); }));
            auto prec = dsl::peek(Common::Keyword<"precedence">::rule) >>
                    (dsl::p<PrecedenceDecl> >>
                     lexy::callback<ItemVariant>(
                             [](auto v) { return std::make_pair(Common::Keyword<"precedence">{}, std::move(v)); }));
            auto pol = dsl::peek(Common::Keyword<"policy">::rule) >>
                    (dsl::p<PolicyDecl> >>
                     lexy::callback<ItemVariant>([](auto v)
                                                 { return std::make_pair(Common::Keyword<"policy">{}, v); }));
            return cond | slice | prec | pol;
        }();
        static constexpr auto value = lexy::forward<ItemVariant>;
    };

    static constexpr auto rule = Common::Keyword<"aggregate">::rule >>
            dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePipeline>(
            [](std::pmr::vector<ItemVariant> items)
            {
                Ast::CallingConvDef::AggregatePipeline pipeline{};
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&v)
                            {
                                using T = std::decay_t<decltype(v)>;
                                if constexpr (std::is_same_v<T,
                                                             std::pmr::vector<Ast::CallingConvDef::AggregateCondition>>)
                                {
                                    for (auto &c : v)
                                        pipeline.m_conditions.push_back(std::move(c));
                                }
                                else if constexpr (std::is_same_v<T,
                                                                  std::pair<Common::Keyword<"slice">,
                                                                            Ast::Common::IntegerLiteral>>)
                                {
                                    pipeline.m_sliceChunkSize = v.second;
                                }
                                else if constexpr (std::is_same_v<T,
                                                                  std::pair<Common::Keyword<"precedence">,
                                                                            std::pmr::vector<Ast::Common::Identifier>>>)
                                {
                                    pipeline.m_mergePrecedence = std::move(v.second);
                                }
                                else if constexpr (std::is_same_v<T,
                                                                  std::pair<Common::Keyword<"policy">,
                                                                            Ast::CallingConvDef::AllocPolicy>>)
                                {
                                    pipeline.m_policy = v.second;
                                }
                            },
                            item);
                }
                return pipeline;
            });
};

struct ClassifySection
{
    static constexpr auto whitespace = Common::Whitespace;

    using EntryVariant = std::variant<Ast::CallingConvDef::PrimitiveRule, Ast::CallingConvDef::AggregatePipeline>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto prim = dsl::peek(Common::Keyword<"types">::rule) >>
                    (dsl::p<PrimitiveRuleParser> >> lexy::construct<EntryVariant>);
            auto agg = dsl::else_ >> (dsl::p<AggregatePipelineParser> >> lexy::construct<EntryVariant>);
            return prim | agg;
        }();
        static constexpr auto value = lexy::forward<EntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"classify">::rule >> dsl::curly_bracketed.list(dsl::p<Entry>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::ClassificationDef>(
            [](std::pmr::vector<EntryVariant> entries)
            {
                Ast::CallingConvDef::ClassificationDef def{};
                for (auto &e : entries)
                {
                    if (std::holds_alternative<Ast::CallingConvDef::PrimitiveRule>(e))
                        def.m_primitives.push_back(std::get<Ast::CallingConvDef::PrimitiveRule>(std::move(e)));
                    else
                        def.m_aggregate = std::get<Ast::CallingConvDef::AggregatePipeline>(std::move(e));
                }
                return def;
            });
};

struct SeqOrConsecutiveRule
{
    static constexpr auto whitespace = Common::Whitespace;

    static constexpr auto rule = []
    {
        auto seq = Common::Keyword<"seq">::rule >>
                (dsl::parenthesized(dsl::p<RegisterListRule>) >>
                 lexy::callback<Ast::CallingConvDef::RegisterSequence>(
                         [](std::pmr::vector<Ast::Common::Identifier> r)
                         {
                             return Ast::CallingConvDef::RegisterSequence{
                                 .m_kind = Ast::CallingConvDef::RegisterSequence::Kind::Sequential,
                                 .m_registers = std::move(r)
                             };
                         }));
        auto con = Common::Keyword<"consecutive">::rule >>
                (dsl::parenthesized(dsl::p<RegisterListRule>) >>
                 lexy::callback<Ast::CallingConvDef::RegisterSequence>(
                         [](std::pmr::vector<Ast::Common::Identifier> r)
                         {
                             return Ast::CallingConvDef::RegisterSequence{
                                 .m_kind = Ast::CallingConvDef::RegisterSequence::Kind::ConsecutiveBlock,
                                 .m_registers = std::move(r)
                             };
                         }));
        return seq | con;
    }();

    static constexpr auto value = lexy::forward<Ast::CallingConvDef::RegisterSequence>;
};

struct PassRuleParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct PassSourceSpec
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto seq = dsl::peek(Common::Keyword<"seq">::rule | Common::Keyword<"consecutive">::rule) >>
                    (dsl::p<SeqOrConsecutiveRule> >> lexy::construct<std::variant<Ast::CallingConvDef::RegisterSequence,
                                                                                  Ast::Common::Identifier,
                                                                                  std::monostate>>);
            auto stack = dsl::peek(Common::Keyword<"stack">::rule) >>
                    (dsl::p<StackFallbackRule> >>
                     lexy::callback<std::variant<Ast::CallingConvDef::RegisterSequence,
                                                 Ast::Common::Identifier,
                                                 std::monostate>>([](auto...) { return std::monostate{}; }));
            auto alias = dsl::else_ >>
                    (dsl::p<Common::Identifier> >> lexy::construct<std::variant<Ast::CallingConvDef::RegisterSequence,
                                                                                Ast::Common::Identifier,
                                                                                std::monostate>>);
            return seq | stack | alias;
        }();
        static constexpr auto value = lexy::forward<
                std::variant<Ast::CallingConvDef::RegisterSequence, Ast::Common::Identifier, std::monostate>>;
    };

    struct FallbackOpt
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::opt(dsl::lit_c<','> >> Common::Keyword<"fallback">::rule >> dsl::lit_c<':'> >>
                                              dsl::p<StackFallbackRule>);
        static constexpr auto value = lexy::forward<std::optional<Ast::CallingConvDef::StackFallback>>;
    };

    static constexpr auto rule = Common::Keyword<"pass">::rule >>
            (dsl::p<Common::Identifier> + LEXY_LIT("=>") + dsl::p<PassSourceSpec> + dsl::p<FallbackOpt>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::PassRule>(
            [](Ast::Common::Identifier abiClass,
               std::variant<Ast::CallingConvDef::RegisterSequence, Ast::Common::Identifier, std::monostate> src,
               std::optional<Ast::CallingConvDef::StackFallback> fb)
            {
                return Ast::CallingConvDef::PassRule{ .m_abiClass = std::move(abiClass),
                                                      .m_source = std::move(src),
                                                      .m_fallback = fb };
            });
};

struct SlotBlockParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct Binding
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::SlotBinding>(
                [](Ast::Common::Identifier cls, Ast::Common::Identifier reg) {
                    return Ast::CallingConvDef::SlotBinding{ .m_abiClass = std::move(cls),
                                                             .m_register = std::move(reg) };
                });
    };

    struct UnifiedSlotRule
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Binding>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::UnifiedSlot>(
                [](std::pmr::vector<Ast::CallingConvDef::SlotBinding> bindings)
                { return Ast::CallingConvDef::UnifiedSlot{ .m_bindings = std::move(bindings) }; });
    };

    struct SlotPayload
    {
        std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> m_slots;
        std::optional<Ast::CallingConvDef::StackFallback> m_fallback;
    };

    static constexpr auto rule = Common::Keyword<"slots">::rule >>
            (dsl::square_bracketed.list(dsl::p<UnifiedSlotRule>, dsl::sep(dsl::lit_c<','>)) +
             dsl::opt(dsl::lit_c<','> >> Common::Keyword<"fallback">::rule >> dsl::lit_c<':'> >>
                      dsl::p<StackFallbackRule>));

    static constexpr auto value = lexy::callback<SlotPayload>(
            [](std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> slots,
               std::optional<Ast::CallingConvDef::StackFallback> fb)
            { return SlotPayload{ .m_slots = std::move(slots), .m_fallback = fb }; },
            [](std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> slots, lexy::nullopt)
            { return SlotPayload{ .m_slots = std::move(slots), .m_fallback = std::nullopt }; },
            [](std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> slots, auto...)
            { return SlotPayload{ .m_slots = std::move(slots), .m_fallback = std::nullopt }; });
};

struct ArgumentsSection
{
    static constexpr auto whitespace = Common::Whitespace;

    using ArgEntryVariant = std::variant<SlotBlockParser::SlotPayload, Ast::CallingConvDef::PassRule>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sl = dsl::peek(Common::Keyword<"slots">::rule) >>
                    (dsl::p<SlotBlockParser> >> lexy::construct<ArgEntryVariant>);
            auto ps = dsl::else_ >> (dsl::p<PassRuleParser> >> lexy::construct<ArgEntryVariant>);
            return sl | ps;
        }();
        static constexpr auto value = lexy::forward<ArgEntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"arguments">::rule >> dsl::curly_bracketed.list(dsl::p<Entry>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::ArgumentPassingDef>(
            [](std::pmr::vector<ArgEntryVariant> entries)
            {
                Ast::CallingConvDef::ArgumentPassingDef def{};
                for (auto &e : entries)
                {
                    if (std::holds_alternative<SlotBlockParser::SlotPayload>(e))
                    {
                        auto &payload = std::get<SlotBlockParser::SlotPayload>(e);
                        def.m_unifiedSlots = std::move(payload.m_slots);
                        def.m_defaultStackFallback = payload.m_fallback;
                    }
                    else
                    {
                        def.m_rules.push_back(std::get<Ast::CallingConvDef::PassRule>(std::move(e)));
                    }
                }
                return def;
            });
};

struct SretDefParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct PtrDecl
    {
        static constexpr auto rule = Common::Keyword<"ptr">::rule >> (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    };
    struct ConsumesDecl
    {
        static constexpr auto rule = Common::Keyword<"consumes_slot">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>);
    };
    struct RetRegDecl
    {
        static constexpr auto rule = Common::Keyword<"returns">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    };

    using Field = std::variant<std::pair<Common::Keyword<"ptr">, Ast::Common::Identifier>,
                               std::pair<Common::Keyword<"consumes_slot">, Ast::Common::BooleanLiteral>,
                               std::pair<Common::Keyword<"returns">, Ast::Common::Identifier>>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto ptr = dsl::peek(Common::Keyword<"ptr">::rule) >>
                    (dsl::p<PtrDecl> >>
                     lexy::callback<Field>([](auto v)
                                           { return std::make_pair(Common::Keyword<"ptr">{}, std::move(v)); }));
            auto con = dsl::peek(Common::Keyword<"consumes_slot">::rule) >>
                    (dsl::p<ConsumesDecl> >>
                     lexy::callback<Field>([](auto v)
                                           { return std::make_pair(Common::Keyword<"consumes_slot">{}, v); }));
            auto ret = dsl::peek(Common::Keyword<"returns">::rule) >>
                    (dsl::p<RetRegDecl> >>
                     lexy::callback<Field>([](auto v)
                                           { return std::make_pair(Common::Keyword<"returns">{}, std::move(v)); }));
            return ptr | con | ret;
        }();
        static constexpr auto value = lexy::forward<Field>;
    };

    static constexpr auto rule = Common::Keyword<"sret">::rule >>
            dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::StructReturnDef>(
            [](std::pmr::vector<Field> fields)
            {
                Ast::CallingConvDef::StructReturnDef sret{};
                for (auto &f : fields)
                {
                    std::visit(
                            [&](auto &&item)
                            {
                                using T = std::decay_t<decltype(item.first)>;
                                if constexpr (std::is_same_v<T, Common::Keyword<"ptr">>)
                                    sret.m_pointerRegister = std::move(item.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"consumes_slot">>)
                                    sret.m_consumesArgSlot = item.second.m_value;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"returns">>)
                                    sret.m_returnRegister = std::move(item.second);
                            },
                            f);
                }
                return sret;
            });
};

struct ReturnsSection
{
    static constexpr auto whitespace = Common::Whitespace;

    using EntryVariant = std::variant<Ast::CallingConvDef::StructReturnDef, Ast::CallingConvDef::PassRule>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sr = dsl::peek(Common::Keyword<"sret">::rule) >>
                    (dsl::p<SretDefParser> >> lexy::construct<EntryVariant>);
            auto ps = dsl::else_ >> (dsl::p<PassRuleParser> >> lexy::construct<EntryVariant>);
            return sr | ps;
        }();
        static constexpr auto value = lexy::forward<EntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"returns">::rule >> dsl::curly_bracketed.list(dsl::p<Entry>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::ReturnDef>(
            [](std::pmr::vector<EntryVariant> entries)
            {
                Ast::CallingConvDef::ReturnDef def{};
                for (auto &e : entries)
                {
                    if (std::holds_alternative<Ast::CallingConvDef::StructReturnDef>(e))
                        def.m_sret = std::get<Ast::CallingConvDef::StructReturnDef>(std::move(e));
                    else
                        def.m_rules.push_back(std::get<Ast::CallingConvDef::PassRule>(std::move(e)));
                }
                return def;
            });
};

struct CalleeSavedPreserve
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"preserve">::rule >>
            (Common::Keyword<"callee">::rule >> dsl::lit_c<':'> >> dsl::p<RegisterListRule>);
    static constexpr auto value = lexy::forward<std::pmr::vector<Ast::Common::Identifier>>;
};

struct CallerSavedPreserve
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"preserve">::rule >>
            (Common::Keyword<"caller">::rule >> dsl::lit_c<':'> >> dsl::p<RegisterListRule>);
    static constexpr auto value = lexy::forward<std::pmr::vector<Ast::Common::Identifier>>;
};

struct CallingConventionBlock
{
    static constexpr auto whitespace = Common::Whitespace;

    using TopLevelItem = std::variant<Ast::CallingConvDef::StackDef,
                                      std::pair<Common::Keyword<"callee">, std::pmr::vector<Ast::Common::Identifier>>,
                                      std::pair<Common::Keyword<"caller">, std::pmr::vector<Ast::Common::Identifier>>,
                                      Ast::CallingConvDef::ClassificationDef,
                                      Ast::CallingConvDef::ArgumentPassingDef,
                                      Ast::CallingConvDef::ReturnDef>;

    struct BodyEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto stack = dsl::peek(Common::Keyword<"stack">::rule) >>
                    (dsl::p<StackSection> >> lexy::construct<TopLevelItem>);
            auto callee = dsl::peek(Common::Keyword<"preserve">::rule >> Common::Keyword<"callee">::rule) >>
                    (dsl::p<CalleeSavedPreserve> >>
                     lexy::callback<TopLevelItem>(
                             [](auto v) { return std::make_pair(Common::Keyword<"callee">{}, std::move(v)); }));
            auto caller = dsl::peek(Common::Keyword<"preserve">::rule >> Common::Keyword<"caller">::rule) >>
                    (dsl::p<CallerSavedPreserve> >>
                     lexy::callback<TopLevelItem>(
                             [](auto v) { return std::make_pair(Common::Keyword<"caller">{}, std::move(v)); }));
            auto cls = dsl::peek(Common::Keyword<"classify">::rule) >>
                    (dsl::p<ClassifySection> >> lexy::construct<TopLevelItem>);
            auto args = dsl::peek(Common::Keyword<"arguments">::rule) >>
                    (dsl::p<ArgumentsSection> >> lexy::construct<TopLevelItem>);
            auto rets = dsl::peek(Common::Keyword<"returns">::rule) >>
                    (dsl::p<ReturnsSection> >> lexy::construct<TopLevelItem>);

            return stack | callee | caller | cls | args | rets;
        }();
        static constexpr auto value = lexy::forward<TopLevelItem>;
    };

    static constexpr auto rule =
            dsl::terminator(dsl::eof).opt(Common::Keyword<"calling_convention">::rule >>
                                          (dsl::p<Common::Identifier> + dsl::curly_bracketed.list(dsl::p<BodyEntry>)));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::CallingConventionDefFile>(
            [](Ast::Common::Identifier name, std::pmr::vector<TopLevelItem> items)
            {
                Ast::CallingConvDef::CallingConventionDef def{};
                def.m_name = std::move(name);

                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::CallingConvDef::StackDef>)
                                    def.m_stack = std::move(val);
                                else if constexpr (std::is_same_v<T,
                                                                  std::pair<Common::Keyword<"callee">,
                                                                            std::pmr::vector<Ast::Common::Identifier>>>)
                                    def.m_calleeSaved = std::move(val.second);
                                else if constexpr (std::is_same_v<T,
                                                                  std::pair<Common::Keyword<"caller">,
                                                                            std::pmr::vector<Ast::Common::Identifier>>>)
                                    def.m_callerSaved = std::move(val.second);
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::ClassificationDef>)
                                    def.m_classification = std::move(val);
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::ArgumentPassingDef>)
                                    def.m_arguments = std::move(val);
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::ReturnDef>)
                                    def.m_returns = std::move(val);
                            },
                            item);
                }
                return def;
            },
            [](auto...) { return Ast::CallingConvDef::CallingConventionDefFile{}; });
};

struct CallingConvDefFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<CallingConventionBlock>;
    static constexpr auto value = lexy::construct<Ast::CallingConvDef::CallingConventionDefFile>;
};

} // namespace DSL::Parser::CallingConvDef

#endif // EZDSL_CALLING_CONV_DEF_LANG_H