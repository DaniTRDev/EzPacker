#ifndef EZDSL_CALLING_CONV_DEF_LANG_H
#define EZDSL_CALLING_CONV_DEF_LANG_H

#include "Ast/CallingConvDefLangAst.h"
#include "Ast/CommonAstNodes.h"
#include "EzDslLexerCommon.h"
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
                [](Ast::Common::IntegerLiteral slot, Ast::Common::IntegerLiteral align)
                {
                    return Ast::CallingConvDef::StackFallback{ .m_slotSize = std::move(slot),
                                                               .m_alignment = std::move(align) };
                },
                [](Ast::Common::IntegerLiteral slot, lexy::nullopt)
                {
                    return Ast::CallingConvDef::StackFallback{ .m_slotSize = std::move(slot),
                                                               .m_alignment = std::nullopt };
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

    using FieldVariant = std::variant<std::pair<Common::Keyword<"align">, Ast::Common::IntegerLiteral>,
                                      std::pair<Common::Keyword<"growth">, Ast::CallingConvDef::StackGrowth>,
                                      std::pair<Common::Keyword<"cleanup">, Ast::CallingConvDef::StackCleanup>,
                                      std::pair<Common::Keyword<"shadow_space">, Ast::Common::IntegerLiteral>,
                                      std::pair<Common::Keyword<"red_zone">, Ast::Common::IntegerLiteral>,
                                      std::pair<Common::Keyword<"sp">, Ast::Common::Identifier>,
                                      std::pair<Common::Keyword<"fp">, Ast::Common::Identifier>,
                                      std::pair<Common::Keyword<"lr">, Ast::Common::Identifier>>;

    struct AlignDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"align">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::IntegerLiteral lit)
                { return std::make_pair(Common::Keyword<"align">{}, lit); });
    };

    struct GrowthDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"growth">::rule >>
                (dsl::lit_c<':'> >> dsl::p<StackGrowthRule>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::CallingConvDef::StackGrowth g)
                { return std::make_pair(Common::Keyword<"growth">{}, g); });
    };

    struct CleanupDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"cleanup">::rule >>
                (dsl::lit_c<':'> >> dsl::p<StackCleanupRule>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::CallingConvDef::StackCleanup c)
                { return std::make_pair(Common::Keyword<"cleanup">{}, c); });
    };

    struct ShadowDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"shadow_space">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::IntegerLiteral lit)
                { return std::make_pair(Common::Keyword<"shadow_space">{}, lit); });
    };

    struct RedZoneDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"red_zone">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::IntegerLiteral lit)
                { return std::make_pair(Common::Keyword<"red_zone">{}, lit); });
    };

    struct SpDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"sp">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::Identifier id)
                { return std::make_pair(Common::Keyword<"sp">{}, std::move(id)); });
    };

    struct FpDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"fp">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::Identifier id)
                { return std::make_pair(Common::Keyword<"fp">{}, std::move(id)); });
    };

    struct LrDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"lr">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<FieldVariant>(
                [](Ast::Common::Identifier id)
                { return std::make_pair(Common::Keyword<"lr">{}, std::move(id)); });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto a = dsl::peek(Common::Keyword<"align">::rule) >> dsl::p<AlignDecl>;
            auto g = dsl::peek(Common::Keyword<"growth">::rule) >> dsl::p<GrowthDecl>;
            auto c = dsl::peek(Common::Keyword<"cleanup">::rule) >> dsl::p<CleanupDecl>;
            auto s = dsl::peek(Common::Keyword<"shadow_space">::rule) >> dsl::p<ShadowDecl>;
            auto rz = dsl::peek(Common::Keyword<"red_zone">::rule) >> dsl::p<RedZoneDecl>;
            auto sp = dsl::peek(Common::Keyword<"sp">::rule) >> dsl::p<SpDecl>;
            auto fp = dsl::peek(Common::Keyword<"fp">::rule) >> dsl::p<FpDecl>;
            auto lr = dsl::peek(Common::Keyword<"lr">::rule) >> dsl::p<LrDecl>;
            return a | g | c | s | rz | sp | fp | lr;
        }();
        static constexpr auto value = lexy::forward<FieldVariant>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<FieldVariant>;
    };

    static constexpr auto rule = Common::Keyword<"stack">::rule >> dsl::p<EntryList>;

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
                                else if constexpr (std::is_same_v<T, Common::Keyword<"red_zone">>)
                                    def.m_redZone = item.second;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"sp">>)
                                    def.m_stackPointer = std::move(item.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"fp">>)
                                    def.m_framePointer = std::move(item.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"lr">>)
                                    def.m_linkRegister = std::move(item.second);
                            },
                            f);
                }
                return def;
            });
};

// =============================================================================
// 2. CLASSIFY BLOCK
// =============================================================================

struct PrimitiveRuleParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct TypeList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::square_bracketed.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Ast::Common::Identifier>;
    };

    static constexpr auto rule = Common::Keyword<"types">::rule >>
            (dsl::p<TypeList> + LEXY_LIT("=>") + dsl::p<Common::Identifier>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::PrimitiveRule>(
            [](std::pmr::vector<Ast::Common::Identifier> types, Ast::Common::Identifier target)
            {
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
                { return std::make_pair(Ast::Common::Identifier{ "by_ref", nullptr }, copy.m_node); },
                [](lexy::nullopt) { return std::make_pair(Ast::Common::Identifier{ "by_ref", nullptr }, false); });
    };

    struct IdSpec
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::callback<std::pair<Ast::Common::Identifier, bool>>(
                [](Ast::Common::Identifier target)
                { return std::make_pair(std::move(target), false); });
    };

    static constexpr auto rule = []
    {
        auto byRef = dsl::peek(Common::Keyword<"by_ref">::rule) >> dsl::p<ByRefSpec>;
        auto id = dsl::else_ >> dsl::p<IdSpec>;
        return byRef | id;
    }();

    static constexpr auto value = lexy::forward<std::pair<Ast::Common::Identifier, bool>>;
};

struct AggregateConditionParser
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SizeCond
    {
        static constexpr auto whitespace = Common::Whitespace;

        struct GtBranch
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = dsl::lit_c<'>'> >> dsl::p<Common::IntegerLiteral>;
            static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                    [](Ast::Common::IntegerLiteral val)
                    {
                        return Ast::CallingConvDef::AggregateCondition{
                            .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeGreaterThan,
                            .m_sizeLimit = val
                        };
                    });
        };

        struct LeBranch
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = LEXY_LIT("<=") >> dsl::p<Common::IntegerLiteral>;
            static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                    [](Ast::Common::IntegerLiteral val)
                    {
                        return Ast::CallingConvDef::AggregateCondition{
                            .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeLessThanOrEqual,
                            .m_sizeLimit = val
                        };
                    });
        };

        struct InBranch
        {
            static constexpr auto whitespace = Common::Whitespace;
            struct SizeList
            {
                static constexpr auto whitespace = Common::Whitespace;
                static constexpr auto rule = dsl::square_bracketed.list(dsl::p<Common::IntegerLiteral>, dsl::sep(dsl::lit_c<','>));
                static constexpr auto value = Common::PmrAsList<Ast::Common::IntegerLiteral>;
            };

            static constexpr auto rule = Common::Keyword<"in">::rule >> dsl::p<SizeList>;
            static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                    [](std::pmr::vector<Ast::Common::IntegerLiteral> sizes)
                    {
                        return Ast::CallingConvDef::AggregateCondition{
                            .m_kind = Ast::CallingConvDef::AggregateCondition::Kind::SizeIn,
                            .m_sizeSet = std::move(sizes)
                        };
                    });
        };

        static constexpr auto rule = Common::Keyword<"size">::rule >>
                ((dsl::peek(dsl::lit_c<'>'>) >> dsl::p<GtBranch>) |
                 (dsl::peek(LEXY_LIT("<=")) >> dsl::p<LeBranch>) |
                 (dsl::peek(Common::Keyword<"in">::rule) >> dsl::p<InBranch>));
        static constexpr auto value = lexy::forward<Ast::CallingConvDef::AggregateCondition>;
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
        static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::AggregateCondition::Kind>
            .map(LEXY_LIT("non_trivial"), Ast::CallingConvDef::AggregateCondition::Kind::NonTrivial)
            .map(LEXY_LIT("unaligned"),   Ast::CallingConvDef::AggregateCondition::Kind::Unaligned);

        static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateCondition>(
                [](Ast::CallingConvDef::AggregateCondition::Kind kind)
                { return Ast::CallingConvDef::AggregateCondition{ .m_kind = kind }; });
    };

    struct CondTerm
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto size = dsl::peek(Common::Keyword<"size">::rule) >> dsl::p<SizeCond>;
            auto hfa = dsl::peek(Common::Keyword<"hfa">::rule | Common::Keyword<"hva">::rule) >> dsl::p<HfaHva>;
            auto flag = dsl::else_ >> dsl::p<SimpleFlags>;
            return size | hfa | flag;
        }();
        static constexpr auto value = lexy::forward<Ast::CallingConvDef::AggregateCondition>;
    };

    struct CondTermList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<CondTerm>, dsl::sep(LEXY_LIT("||")));
        static constexpr auto value = Common::PmrAsList<Ast::CallingConvDef::AggregateCondition>;
    };

    // Parses: when <cond> (|| <cond>)* => <target>
    struct WhenBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"when">::rule >>
                (dsl::p<CondTermList> + LEXY_LIT("=>") + dsl::p<TargetClassSpec>);

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

    using ItemVariant =
            std::variant<std::pmr::vector<Ast::CallingConvDef::AggregateCondition>,
                         std::pair<Common::Keyword<"slice">, Ast::Common::IntegerLiteral>,
                         std::pair<Common::Keyword<"precedence">, std::pmr::vector<Ast::Common::Identifier>>,
                         std::pair<Common::Keyword<"policy">, Ast::CallingConvDef::AllocPolicy>>;

    struct CondDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<AggregateConditionParser>;
        static constexpr auto value = lexy::callback<ItemVariant>(
                [](std::pmr::vector<Ast::CallingConvDef::AggregateCondition> conds)
                { return ItemVariant{ std::move(conds) }; });
    };

    struct SliceDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"slice">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral> >> dsl::opt(Common::Keyword<"bytes">::rule));
        static constexpr auto value = lexy::callback<ItemVariant>(
                [](Ast::Common::IntegerLiteral lit, auto...)
                { return std::make_pair(Common::Keyword<"slice">{}, lit); });
    };

    struct PrecedenceDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        struct PrecedenceList
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = dsl::square_bracketed.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
            static constexpr auto value = Common::PmrAsList<Ast::Common::Identifier>;
        };

        static constexpr auto rule = Common::Keyword<"precedence">::rule >>
                (dsl::lit_c<':'> >> dsl::p<PrecedenceList>);
        static constexpr auto value = lexy::callback<ItemVariant>(
                [](std::pmr::vector<Ast::Common::Identifier> v)
                { return std::make_pair(Common::Keyword<"precedence">{}, std::move(v)); });
    };

    struct PolicyDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"policy">::rule >> (dsl::lit_c<':'> >> dsl::p<AllocPolicyRule>);
        static constexpr auto value = lexy::callback<ItemVariant>(
                [](Ast::CallingConvDef::AllocPolicy pol)
                { return std::make_pair(Common::Keyword<"policy">{}, pol); });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto cond = dsl::peek(Common::Keyword<"when">::rule | Common::Keyword<"default">::rule) >> dsl::p<CondDecl>;
            auto slice = dsl::peek(Common::Keyword<"slice">::rule) >> dsl::p<SliceDecl>;
            auto prec = dsl::peek(Common::Keyword<"precedence">::rule) >> dsl::p<PrecedenceDecl>;
            auto pol = dsl::peek(Common::Keyword<"policy">::rule) >> dsl::p<PolicyDecl>;
            return cond | slice | prec | pol;
        }();
        static constexpr auto value = lexy::forward<ItemVariant>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<ItemVariant>;
    };

    static constexpr auto rule = Common::Keyword<"aggregate">::rule >> dsl::p<EntryList>;

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

    struct PrimDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<PrimitiveRuleParser>;
        static constexpr auto value = lexy::callback<EntryVariant>(
                [](Ast::CallingConvDef::PrimitiveRule r) { return EntryVariant{ std::move(r) }; });
    };

    struct AggDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<AggregatePipelineParser>;
        static constexpr auto value = lexy::callback<EntryVariant>(
                [](Ast::CallingConvDef::AggregatePipeline p) { return EntryVariant{ std::move(p) }; });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto prim = dsl::peek(Common::Keyword<"types">::rule) >> dsl::p<PrimDecl>;
            auto agg = dsl::else_ >> dsl::p<AggDecl>;
            return prim | agg;
        }();
        static constexpr auto value = lexy::forward<EntryVariant>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<EntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"classify">::rule >> dsl::p<EntryList>;

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

// =============================================================================
// 3. ARGUMENTS AND RETURNS BLOCKS
// =============================================================================

struct SeqOrConsecutiveRule
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SeqRule
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"seq">::rule >>
                dsl::parenthesized(dsl::p<RegisterListRule>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::RegisterSequence>(
                [](std::pmr::vector<Ast::Common::Identifier> r)
                {
                    return Ast::CallingConvDef::RegisterSequence{
                        .m_kind = Ast::CallingConvDef::RegisterSequence::Kind::Sequential,
                        .m_registers = std::move(r)
                    };
                });
    };

    struct ConRule
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"consecutive">::rule >>
                dsl::parenthesized(dsl::p<RegisterListRule>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::RegisterSequence>(
                [](std::pmr::vector<Ast::Common::Identifier> r)
                {
                    return Ast::CallingConvDef::RegisterSequence{
                        .m_kind = Ast::CallingConvDef::RegisterSequence::Kind::ConsecutiveBlock,
                        .m_registers = std::move(r)
                    };
                });
    };

    static constexpr auto rule = []
    {
        auto seq = dsl::peek(Common::Keyword<"seq">::rule) >> dsl::p<SeqRule>;
        auto con = dsl::else_ >> dsl::p<ConRule>;
        return seq | con;
    }();

    static constexpr auto value = lexy::forward<Ast::CallingConvDef::RegisterSequence>;
};

struct PassRuleParser
{
    static constexpr auto whitespace = Common::Whitespace;

    using SourceVariant = std::variant<Ast::CallingConvDef::RegisterSequence,
                                       Ast::Common::Identifier,
                                       std::monostate>;

    struct SeqSource
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<SeqOrConsecutiveRule>;
        static constexpr auto value = lexy::callback<SourceVariant>(
                [](Ast::CallingConvDef::RegisterSequence s) { return SourceVariant{ std::move(s) }; });
    };

    struct StackSource
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<StackFallbackRule>;
        static constexpr auto value = lexy::callback<SourceVariant>(
                [](auto...) { return SourceVariant{ std::monostate{} }; });
    };

    struct AliasSource
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<Common::Identifier>;
        static constexpr auto value = lexy::callback<SourceVariant>(
                [](Ast::Common::Identifier id) { return SourceVariant{ std::move(id) }; });
    };

    struct PassSourceSpec
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto seq = dsl::peek(Common::Keyword<"seq">::rule | Common::Keyword<"consecutive">::rule) >> dsl::p<SeqSource>;
            auto stack = dsl::peek(Common::Keyword<"stack">::rule) >> dsl::p<StackSource>;
            auto alias = dsl::else_ >> dsl::p<AliasSource>;
            return seq | stack | alias;
        }();
        static constexpr auto value = lexy::forward<SourceVariant>;
    };

    struct FallbackOpt
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::opt(dsl::lit_c<','> >> Common::Keyword<"fallback">::rule >> dsl::lit_c<':'> >>
                                              dsl::p<StackFallbackRule>);
        static constexpr auto value = lexy::callback<std::optional<Ast::CallingConvDef::StackFallback>>(
                [](Ast::CallingConvDef::StackFallback fb) { return std::optional(std::move(fb)); },
                [](lexy::nullopt) { return std::nullopt; });
    };

    static constexpr auto rule = Common::Keyword<"pass">::rule >>
            (dsl::p<Common::Identifier> + LEXY_LIT("=>") + dsl::p<PassSourceSpec> + dsl::p<FallbackOpt>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::PassRule>(
            [](Ast::Common::Identifier abiClass,
               SourceVariant src,
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
                [](Ast::Common::Identifier cls, Ast::Common::Identifier reg)
                {
                    return Ast::CallingConvDef::SlotBinding{ .m_abiClass = std::move(cls),
                                                             .m_register = std::move(reg) };
                });
    };

    struct BindingList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Binding>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Ast::CallingConvDef::SlotBinding>;
    };

    struct UnifiedSlotRule
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<BindingList>;
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::UnifiedSlot>(
                [](std::pmr::vector<Ast::CallingConvDef::SlotBinding> bindings)
                { return Ast::CallingConvDef::UnifiedSlot{ .m_bindings = std::move(bindings) }; });
    };

    struct UnifiedSlotList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::square_bracketed.list(dsl::p<UnifiedSlotRule>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Ast::CallingConvDef::UnifiedSlot>;
    };

    struct SlotPayload
    {
        std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> m_slots;
        std::optional<Ast::CallingConvDef::StackFallback> m_fallback;
    };

    static constexpr auto rule = Common::Keyword<"slots">::rule >>
            (dsl::p<UnifiedSlotList> +
             dsl::opt(dsl::lit_c<','> >> Common::Keyword<"fallback">::rule >> dsl::lit_c<':'> >>
                      dsl::p<StackFallbackRule>));

    static constexpr auto value = lexy::callback<SlotPayload>(
            [](std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> slots,
               Ast::CallingConvDef::StackFallback fb)
            { return SlotPayload{ .m_slots = std::move(slots), .m_fallback = std::move(fb) }; },
            [](std::pmr::vector<Ast::CallingConvDef::UnifiedSlot> slots, lexy::nullopt)
            { return SlotPayload{ .m_slots = std::move(slots), .m_fallback = std::nullopt }; });
};

struct ArgumentsSection
{
    static constexpr auto whitespace = Common::Whitespace;

    using ArgEntryVariant = std::variant<SlotBlockParser::SlotPayload, Ast::CallingConvDef::PassRule>;

    struct SlotEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<SlotBlockParser>;
        static constexpr auto value = lexy::callback<ArgEntryVariant>(
                [](SlotBlockParser::SlotPayload p) { return ArgEntryVariant{ std::move(p) }; });
    };

    struct PassEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<PassRuleParser>;
        static constexpr auto value = lexy::callback<ArgEntryVariant>(
                [](Ast::CallingConvDef::PassRule r) { return ArgEntryVariant{ std::move(r) }; });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sl = dsl::peek(Common::Keyword<"slots">::rule) >> dsl::p<SlotEntry>;
            auto ps = dsl::else_ >> dsl::p<PassEntry>;
            return sl | ps;
        }();
        static constexpr auto value = lexy::forward<ArgEntryVariant>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<ArgEntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"arguments">::rule >> dsl::p<EntryList>;

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

    using Field = std::variant<std::pair<Common::Keyword<"ptr">, Ast::Common::Identifier>,
                               std::pair<Common::Keyword<"consumes_slot">, Ast::Common::BooleanLiteral>,
                               std::pair<Common::Keyword<"returns">, Ast::Common::Identifier>>;

    struct PtrDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"ptr">::rule >> (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::Identifier id) { return std::make_pair(Common::Keyword<"ptr">{}, std::move(id)); });
    };

    struct ConsumesDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"consumes_slot">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::BooleanLiteral b) { return std::make_pair(Common::Keyword<"consumes_slot">{}, b); });
    };

    struct RetRegDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"returns">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::Identifier id) { return std::make_pair(Common::Keyword<"returns">{}, std::move(id)); });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto ptr = dsl::peek(Common::Keyword<"ptr">::rule) >> dsl::p<PtrDecl>;
            auto con = dsl::peek(Common::Keyword<"consumes_slot">::rule) >> dsl::p<ConsumesDecl>;
            auto ret = dsl::peek(Common::Keyword<"returns">::rule) >> dsl::p<RetRegDecl>;
            return ptr | con | ret;
        }();
        static constexpr auto value = lexy::forward<Field>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Field>;
    };

    static constexpr auto rule = Common::Keyword<"sret">::rule >> dsl::p<EntryList>;

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
                                    sret.m_consumesArgSlot = item.second.m_node;
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

    struct SretEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<SretDefParser>;
        static constexpr auto value = lexy::callback<EntryVariant>(
                [](Ast::CallingConvDef::StructReturnDef s) { return EntryVariant{ std::move(s) }; });
    };

    struct PassEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<PassRuleParser>;
        static constexpr auto value = lexy::callback<EntryVariant>(
                [](Ast::CallingConvDef::PassRule r) { return EntryVariant{ std::move(r) }; });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sr = dsl::peek(Common::Keyword<"sret">::rule) >> dsl::p<SretEntry>;
            auto ps = dsl::else_ >> dsl::p<PassEntry>;
            return sr | ps;
        }();
        static constexpr auto value = lexy::forward<EntryVariant>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<EntryVariant>;
    };

    static constexpr auto rule = Common::Keyword<"returns">::rule >> dsl::p<EntryList>;

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

// =============================================================================
// 4. PRESERVE LISTS
// =============================================================================

using CallingConvTopLevelItem = std::variant<Ast::CallingConvDef::StackDef,
                                             std::pair<Common::Keyword<"callee">, std::pmr::vector<Ast::Common::Identifier>>,
                                             std::pair<Common::Keyword<"caller">, std::pmr::vector<Ast::Common::Identifier>>,
                                             Ast::CallingConvDef::ClassificationDef,
                                             Ast::CallingConvDef::ArgumentPassingDef,
                                             Ast::CallingConvDef::ReturnDef,
                                             Ast::CallingConvDef::VarargsDef>;

struct PreserveSection
{
    static constexpr auto whitespace = Common::Whitespace;

    struct CalleeBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"callee">::rule >> (dsl::lit_c<':'> >> dsl::p<RegisterListRule>);
        static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
                [](std::pmr::vector<Ast::Common::Identifier> v) {
                    return CallingConvTopLevelItem{ std::make_pair(Common::Keyword<"callee">{}, std::move(v)) };
                });
    };

    struct CallerBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"caller">::rule >> (dsl::lit_c<':'> >> dsl::p<RegisterListRule>);
        static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
                [](std::pmr::vector<Ast::Common::Identifier> v) {
                    return CallingConvTopLevelItem{ std::make_pair(Common::Keyword<"caller">{}, std::move(v)) };
                });
    };

    static constexpr auto rule = Common::Keyword<"preserve">::rule >>
            ((dsl::peek(Common::Keyword<"callee">::rule) >> dsl::p<CalleeBranch>) |
             (dsl::peek(Common::Keyword<"caller">::rule) >> dsl::p<CallerBranch>));
    static constexpr auto value = lexy::forward<CallingConvTopLevelItem>;
};

// =============================================================================
// 5. VARARGS SECTION
// =============================================================================

struct VarargsSection
{
    static constexpr auto whitespace = Common::Whitespace;

    using Field = std::variant<std::pair<Common::Keyword<"vector_count_reg">, Ast::Common::Identifier>,
                               std::pair<Common::Keyword<"duplicate_floats_to_gpr">, Ast::Common::BooleanLiteral>,
                               std::pair<Common::Keyword<"stack_align">, Ast::Common::IntegerLiteral>>;

    struct VectorCountDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"vector_count_reg">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::Identifier id) { return std::make_pair(Common::Keyword<"vector_count_reg">{}, std::move(id)); });
    };

    struct DuplicateFloatsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"duplicate_floats_to_gpr">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::BooleanLiteral b) { return std::make_pair(Common::Keyword<"duplicate_floats_to_gpr">{}, b); });
    };

    struct StackAlignDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"stack_align">::rule >>
                (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
        static constexpr auto value = lexy::callback<Field>(
                [](Ast::Common::IntegerLiteral lit) { return std::make_pair(Common::Keyword<"stack_align">{}, lit); });
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto vc = dsl::peek(Common::Keyword<"vector_count_reg">::rule) >> dsl::p<VectorCountDecl>;
            auto df = dsl::peek(Common::Keyword<"duplicate_floats_to_gpr">::rule) >> dsl::p<DuplicateFloatsDecl>;
            auto sa = dsl::peek(Common::Keyword<"stack_align">::rule) >> dsl::p<StackAlignDecl>;
            return vc | df | sa;
        }();
        static constexpr auto value = lexy::forward<Field>;
    };

    struct EntryList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<Entry>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<Field>;
    };

    static constexpr auto rule = Common::Keyword<"varargs">::rule >> dsl::p<EntryList>;

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::VarargsDef>(
            [](std::pmr::vector<Field> fields)
            {
                Ast::CallingConvDef::VarargsDef def{};
                for (auto &f : fields)
                {
                    std::visit(
                            [&](auto &&item)
                            {
                                using T = std::decay_t<decltype(item.first)>;
                                if constexpr (std::is_same_v<T, Common::Keyword<"vector_count_reg">>)
                                    def.m_vectorCountReg = std::move(item.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"duplicate_floats_to_gpr">>)
                                    def.m_duplicateFloatsToGpr = item.second.m_node;
                                else if constexpr (std::is_same_v<T, Common::Keyword<"stack_align">>)
                                    def.m_stackAlign = item.second;
                            },
                            f);
                }
                return def;
            });
};

// =============================================================================
// 6. TOP LEVEL CALLING CONVENTION
// =============================================================================

struct StackTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<StackSection>;
    static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
            [](Ast::CallingConvDef::StackDef s) { return CallingConvTopLevelItem{ std::move(s) }; });
};

struct ClassifyTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<ClassifySection>;
    static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
            [](Ast::CallingConvDef::ClassificationDef c) { return CallingConvTopLevelItem{ std::move(c) }; });
};

struct ArgsTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<ArgumentsSection>;
    static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
            [](Ast::CallingConvDef::ArgumentPassingDef a) { return CallingConvTopLevelItem{ std::move(a) }; });
};

struct ReturnsTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<ReturnsSection>;
    static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
            [](Ast::CallingConvDef::ReturnDef r) { return CallingConvTopLevelItem{ std::move(r) }; });
};

struct VarargsTopLevel
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<VarargsSection>;
    static constexpr auto value = lexy::callback<CallingConvTopLevelItem>(
            [](Ast::CallingConvDef::VarargsDef v) { return CallingConvTopLevelItem{ std::move(v) }; });
};

struct CallingConvBodyEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = []
    {
        auto stack = dsl::peek(Common::Keyword<"stack">::rule) >> dsl::p<StackTopLevel>;
        auto preserve = dsl::peek(Common::Keyword<"preserve">::rule) >> dsl::p<PreserveSection>;
        auto cls = dsl::peek(Common::Keyword<"classify">::rule) >> dsl::p<ClassifyTopLevel>;
        auto args = dsl::peek(Common::Keyword<"arguments">::rule) >> dsl::p<ArgsTopLevel>;
        auto rets = dsl::peek(Common::Keyword<"returns">::rule) >> dsl::p<ReturnsTopLevel>;
        auto va = dsl::peek(Common::Keyword<"varargs">::rule) >> dsl::p<VarargsTopLevel>;

        return stack | preserve | cls | args | rets | va;
    }();
    static constexpr auto value = lexy::forward<CallingConvTopLevelItem>;
};

struct CallingConventionBlock
{
    static constexpr auto whitespace = Common::Whitespace;

    struct BodyList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<CallingConvBodyEntry>);
        static constexpr auto value = Common::PmrAsList<CallingConvTopLevelItem>;
    };

    static constexpr auto rule = Common::Keyword<"calling_convention">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<BodyList>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::CallingConventionDecl>(
            [](Ast::Common::Identifier name, std::pmr::vector<CallingConvTopLevelItem> items)
            {
                Ast::CallingConvDef::CallingConventionDecl def{};
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
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::VarargsDef>)
                                    def.m_varargs = std::move(val);
                            },
                            item);
                }
                return def;
            });
};

struct CallingConvDefFile
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ConvList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::peek(Common::Keyword<"calling_convention">::rule) >> dsl::p<CallingConventionBlock>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::CallingConvDef::CallingConventionDecl>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof)(dsl::p<ConvList>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::CallingConventionDefFile>(
            [](std::pmr::vector<Ast::CallingConvDef::CallingConventionDecl> decls)
            {
                Ast::CallingConvDef::CallingConventionDefFile file;
                if (!decls.empty())
                {
                    static_cast<Ast::CallingConvDef::CallingConventionDecl &>(file) = decls.front();
                    file.m_conventions = std::move(decls);
                }
                return file;
            });
};

} // namespace DSL::Parser::CallingConvDef

#endif // EZDSL_CALLING_CONV_DEF_LANG_H