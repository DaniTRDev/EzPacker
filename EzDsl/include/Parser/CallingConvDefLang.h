#ifndef EZDSL_CALLING_CONV_DEF_LANG_H
#define EZDSL_CALLING_CONV_DEF_LANG_H

#include <optional>
#include <utility>
#include <variant>
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::CallingConvDef
{
namespace dsl = ::lexy::dsl;

// Matches "DOWN" or "UP"
struct StackDirection
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::StackDirection>
        .map(LEXY_LIT("DOWN"), Ast::CallingConvDef::StackDirection::Down)
        .map(LEXY_LIT("UP"), Ast::CallingConvDef::StackDirection::Up);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackDirection>;
};

// Matches "CALLER" or "CALLEE"
struct StackCleaner
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::StackCleaner>
        .map(LEXY_LIT("CALLER"), Ast::CallingConvDef::StackCleaner::Caller)
        .map(LEXY_LIT("CALLEE"), Ast::CallingConvDef::StackCleaner::Callee);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackCleaner>;
};

// Matches strictly "ALL_OR_NOTHING" or "INDEPENDENT"
struct AllocPolicy
{
    static constexpr auto Table = lexy::symbol_table<Ast::CallingConvDef::AllocPolicy>
        .map(LEXY_LIT("ALL_OR_NOTHING"), Ast::CallingConvDef::AllocPolicy::AllOrNothing)
        .map(LEXY_LIT("INDEPENDENT"), Ast::CallingConvDef::AllocPolicy::Independent);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore));
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::AllocPolicy>;
};

// Matches boolean literals
struct BooleanLit
{
    static constexpr auto Table = lexy::symbol_table<bool>
        .map(LEXY_LIT("true"), true)
        .map(LEXY_LIT("TRUE"), true)
        .map(LEXY_LIT("false"), false)
        .map(LEXY_LIT("FALSE"), false);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<bool>;
};

// Matches "GPR:rdi", "FPR:xmm0"
struct RegisterRef
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier>;

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::RegisterRef>(
            [](Ast::Common::Identifier cls, Ast::Common::Identifier reg)
            { return Ast::CallingConvDef::RegisterRef{ .m_className = std::move(cls), .m_regName = std::move(reg) }; });
};

// Matches "STACK", "STACK(ALIGN: 8)", "STACK(8)"
struct StackPlacement
{
    static constexpr auto whitespace = Common::Whitespace;

    struct AlignSpec
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto withAlign = dsl::peek(Common::Keyword<"ALIGN">::rule) >>
                    (Common::Keyword<"ALIGN">::rule >> (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>));
            auto directNum = dsl::else_ >> dsl::p<Common::IntegerLiteral>;
            return withAlign | directNum;
        }();
        static constexpr auto value = lexy::forward<Ast::Common::IntegerLiteral>;
    };

    static constexpr auto rule = Common::Keyword<"STACK">::rule >> dsl::opt(dsl::parenthesized(dsl::p<AlignSpec>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::StackPlacement>(
            [](Ast::Common::IntegerLiteral align)
            { return Ast::CallingConvDef::StackPlacement{ .m_alignment = align }; },
            [](lexy::nullopt) { return Ast::CallingConvDef::StackPlacement{ .m_alignment = std::nullopt }; },
            [](auto...) { return Ast::CallingConvDef::StackPlacement{ .m_alignment = std::nullopt }; });
};

// =============================================================================
// 1. CLASSIFICATION STAGE
// =============================================================================

// Matches "TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;"
struct PrimitiveClassifyRule
{
    static constexpr auto whitespace = Common::Whitespace;

    struct TypeList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    static constexpr auto rule = Common::Keyword<"TYPE">::rule >>
            (dsl::p<TypeList> + LEXY_LIT(">>") + dsl::p<Common::Identifier> + dsl::lit_c<';'>);

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::PrimitiveClassifyRule>(
            [](std::pmr::vector<Ast::Common::Identifier> types, Ast::Common::Identifier targetClass)
            {
                return Ast::CallingConvDef::PrimitiveClassifyRule{ .m_types = std::move(types),
                                                                   .m_targetClass = std::move(targetClass) };
            });
};

struct AggregatePredicate
{
    static constexpr auto whitespace = Common::Whitespace;

    struct SizeGtBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IF_SIZE_GT">::rule >>
                (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](Ast::Common::IntegerLiteral size, Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::SizeGt,
                        .m_size = size,
                        .m_sizes = {},
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct SizeLeBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IF_SIZE_LE">::rule >>
                (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](Ast::Common::IntegerLiteral size, Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::SizeLe,
                        .m_size = size,
                        .m_sizes = {},
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct SizeInBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        struct SizeList
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule =
                    dsl::parenthesized.list(dsl::p<Common::IntegerLiteral>, dsl::sep(dsl::lit_c<','>));
            static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::IntegerLiteral>>;
        };

        static constexpr auto rule = Common::Keyword<"IF_SIZE_IN">::rule >>
                (dsl::p<SizeList> + LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](std::pmr::vector<Ast::Common::IntegerLiteral> sizes, Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::SizeIn,
                        .m_size = std::nullopt,
                        .m_sizes = std::move(sizes),
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct HomogeneousBranch
    {
        static constexpr auto whitespace = Common::Whitespace;

        struct Args
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = []
            {
                auto maxKw = Common::Keyword<"MAX">::rule;
                auto optMax = dsl::opt(dsl::peek(maxKw) >> (maxKw + dsl::lit_c<':'>));
                return dsl::p<Common::Identifier> + dsl::lit_c<','> + optMax + dsl::p<Common::IntegerLiteral>;
            }();
            static constexpr auto value =
                    lexy::callback<std::pair<Ast::Common::Identifier, Ast::Common::IntegerLiteral>>(
                            [](Ast::Common::Identifier cls, Ast::Common::IntegerLiteral maxElem)
                            { return std::make_pair(std::move(cls), maxElem); },
                            [](Ast::Common::Identifier cls, lexy::nullopt, Ast::Common::IntegerLiteral maxElem)
                            { return std::make_pair(std::move(cls), maxElem); });
        };

        static constexpr auto rule = Common::Keyword<"IF_HOMOGENEOUS">::rule >>
                (dsl::parenthesized(dsl::p<Args>) + LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](std::pair<Ast::Common::Identifier, Ast::Common::IntegerLiteral> args, Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::Homogeneous,
                        .m_size = std::nullopt,
                        .m_sizes = {},
                        .m_homogeneousClass = std::move(args.first),
                        .m_maxElements = args.second,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct UnalignedBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IF_UNALIGNED">::rule >>
                (LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::Unaligned,
                        .m_size = std::nullopt,
                        .m_sizes = {},
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct NonTrivialBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IF_NON_TRIVIAL">::rule >>
                (LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::NonTrivial,
                        .m_size = std::nullopt,
                        .m_sizes = {},
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    struct DefaultBranch
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"DEFAULT">::rule >> (LEXY_LIT(">>") + dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregatePredicate>(
                [](Ast::Common::Identifier res)
                {
                    return Ast::CallingConvDef::AggregatePredicate{
                        .m_kind = Ast::CallingConvDef::AggregatePredicateKind::Default,
                        .m_size = std::nullopt,
                        .m_sizes = {},
                        .m_homogeneousClass = std::nullopt,
                        .m_maxElements = std::nullopt,
                        .m_resultClass = std::move(res)
                    };
                });
    };

    static constexpr auto rule = []
    {
        auto sizeGt = dsl::peek(Common::Keyword<"IF_SIZE_GT">::rule) >> dsl::p<SizeGtBranch>;
        auto sizeLe = dsl::peek(Common::Keyword<"IF_SIZE_LE">::rule) >> dsl::p<SizeLeBranch>;
        auto sizeIn = dsl::peek(Common::Keyword<"IF_SIZE_IN">::rule) >> dsl::p<SizeInBranch>;
        auto homog = dsl::peek(Common::Keyword<"IF_HOMOGENEOUS">::rule) >> dsl::p<HomogeneousBranch>;
        auto unal = dsl::peek(Common::Keyword<"IF_UNALIGNED">::rule) >> dsl::p<UnalignedBranch>;
        auto nontr = dsl::peek(Common::Keyword<"IF_NON_TRIVIAL">::rule) >> dsl::p<NonTrivialBranch>;
        auto def = dsl::peek(Common::Keyword<"DEFAULT">::rule) >> dsl::p<DefaultBranch>;
        return (sizeGt | sizeLe | sizeIn | homog | unal | nontr | def) + dsl::lit_c<';'>;
    }();

    static constexpr auto value = lexy::forward<Ast::CallingConvDef::AggregatePredicate>;
};

struct ChunkSizeDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"CHUNK_SIZE">::rule >>
            (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::Common::IntegerLiteral>;
};

struct MergePrecedenceDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"MERGE_PRECEDENCE">::rule >>
            (LEXY_LIT(">>") + dsl::list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<'>'>)) + dsl::lit_c<';'>);
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
};

struct AllocPolicyDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"ALLOC_POLICY">::rule >>
            (dsl::parenthesized(dsl::p<AllocPolicy>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::AllocPolicy>;
};

struct AggregateClassifyDef
{
    static constexpr auto whitespace = Common::Whitespace;

    using AggregateItem = std::variant<Ast::CallingConvDef::AggregatePredicate,
                                       Ast::Common::IntegerLiteral,
                                       std::pmr::vector<Ast::Common::Identifier>,
                                       Ast::CallingConvDef::AllocPolicy>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto predBranch = dsl::peek(Common::Keyword<"IF_SIZE_GT">::rule | Common::Keyword<"IF_SIZE_LE">::rule |
                                        Common::Keyword<"IF_SIZE_IN">::rule | Common::Keyword<"IF_HOMOGENEOUS">::rule |
                                        Common::Keyword<"IF_UNALIGNED">::rule |
                                        Common::Keyword<"IF_NON_TRIVIAL">::rule | Common::Keyword<"DEFAULT">::rule) >>
                    dsl::p<AggregatePredicate>;
            auto chunkBranch = dsl::peek(Common::Keyword<"CHUNK_SIZE">::rule) >> dsl::p<ChunkSizeDecl>;
            auto precBranch = dsl::peek(Common::Keyword<"MERGE_PRECEDENCE">::rule) >> dsl::p<MergePrecedenceDecl>;
            auto allocBranch = dsl::peek(Common::Keyword<"ALLOC_POLICY">::rule) >> dsl::p<AllocPolicyDecl>;
            return predBranch | chunkBranch | precBranch | allocBranch;
        }();

        static constexpr auto value = lexy::construct<AggregateItem>;
    };

    struct ItemList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<AggregateItem>>;
    };

    static constexpr auto rule = Common::Keyword<"AGGREGATE">::rule >> (dsl::p<ItemList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::AggregateClassifyDef>(
            [](std::pmr::vector<AggregateItem> items, auto...)
            {
                Ast::CallingConvDef::AggregateClassifyDef aggDef;
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::CallingConvDef::AggregatePredicate>)
                                {
                                    aggDef.m_predicates.push_back(std::move(val));
                                }
                                else if constexpr (std::is_same_v<T, Ast::Common::IntegerLiteral>)
                                {
                                    aggDef.m_chunkSize = val;
                                }
                                else if constexpr (std::is_same_v<T, std::pmr::vector<Ast::Common::Identifier>>)
                                {
                                    aggDef.m_mergePrecedence = std::move(val);
                                }
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::AllocPolicy>)
                                {
                                    aggDef.m_allocPolicy = val;
                                }
                            },
                            item);
                }
                return aggDef;
            });
};

struct ClassifyBlock
{
    static constexpr auto whitespace = Common::Whitespace;

    using ClassifyItem =
            std::variant<Ast::CallingConvDef::PrimitiveClassifyRule, Ast::CallingConvDef::AggregateClassifyDef>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto typeBranch = dsl::peek(Common::Keyword<"TYPE">::rule) >> dsl::p<PrimitiveClassifyRule>;
            auto aggBranch = dsl::peek(Common::Keyword<"AGGREGATE">::rule) >> dsl::p<AggregateClassifyDef>;
            return typeBranch | aggBranch;
        }();

        static constexpr auto value = lexy::construct<ClassifyItem>;
    };

    struct ItemList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<ClassifyItem>>;
    };

    static constexpr auto rule = Common::Keyword<"CLASSIFY">::rule >> (dsl::p<ItemList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::ClassifyBlock>(
            [](std::pmr::vector<ClassifyItem> items, auto...)
            {
                Ast::CallingConvDef::ClassifyBlock block;
                for (auto &item : items)
                {
                    if (std::holds_alternative<Ast::CallingConvDef::PrimitiveClassifyRule>(item))
                    {
                        block.m_primitiveRules.push_back(
                                std::get<Ast::CallingConvDef::PrimitiveClassifyRule>(std::move(item)));
                    }
                    else if (std::holds_alternative<Ast::CallingConvDef::AggregateClassifyDef>(item))
                    {
                        block.m_aggregateDef = std::get<Ast::CallingConvDef::AggregateClassifyDef>(std::move(item));
                    }
                }
                return block;
            });
};

// =============================================================================
// 2. DISPATCH & SRET PARSERS
// =============================================================================

struct RegRefList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::parenthesized.list(dsl::p<RegisterRef>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::CallingConvDef::RegisterRef>>;
};

struct LoweringAction
{
    static constexpr auto whitespace = Common::Whitespace;

    struct ActionRegSeq
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"REG_SEQ">::rule >>
                (dsl::p<RegRefList> + dsl::opt(LEXY_LIT(">>") >> dsl::p<StackPlacement>));

        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, Ast::CallingConvDef::StackPlacement stack)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Sequence,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = stack
                    };
                },
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, lexy::nullopt)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Sequence,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = std::nullopt
                    };
                },
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, auto...)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Sequence,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = std::nullopt
                    };
                });
    };

    struct ActionRegSlots
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"REG_SLOTS">::rule >>
                (dsl::p<RegRefList> + dsl::opt(LEXY_LIT(">>") >> dsl::p<StackPlacement>));

        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, Ast::CallingConvDef::StackPlacement stack)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Slots,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = stack
                    };
                },
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, lexy::nullopt)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Slots,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = std::nullopt
                    };
                },
                [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs, auto...)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::RegisterAssign,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Slots,
                        .m_registers = std::move(regs),
                        .m_targetClass = std::nullopt,
                        .m_stackFallback = std::nullopt
                    };
                });
    };

    struct ActionExpandTo
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"EXPAND_TO">::rule >>
                dsl::parenthesized(dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                [](Ast::Common::Identifier target)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::ExpandTo,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Sequence,
                        .m_registers = {},
                        .m_targetClass = std::move(target),
                        .m_stackFallback = std::nullopt
                    };
                });
    };

    struct ActionPassAsPointer
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"PASS_AS_POINTER">::rule >>
                (LEXY_LIT(">>") >> dsl::p<Common::Identifier>);
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                [](Ast::Common::Identifier target)
                {
                    return Ast::CallingConvDef::LoweringAction{
                        .m_kind = Ast::CallingConvDef::LoweringActionKind::PassAsPointer,
                        .m_regAssignKind = Ast::CallingConvDef::RegAssignKind::Sequence,
                        .m_registers = {},
                        .m_targetClass = std::move(target),
                        .m_stackFallback = std::nullopt
                    };
                });
    };

    struct ActionStack
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<StackPlacement>;
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                [](Ast::CallingConvDef::StackPlacement placement)
                {
                    return Ast::CallingConvDef::LoweringAction{ .m_kind =
                                                                        Ast::CallingConvDef::LoweringActionKind::Stack,
                                                                .m_regAssignKind =
                                                                        Ast::CallingConvDef::RegAssignKind::Sequence,
                                                                .m_registers = {},
                                                                .m_targetClass = std::nullopt,
                                                                .m_stackFallback = placement };
                });
    };

    struct ActionSret
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"SRET">::rule;
        static constexpr auto value = lexy::callback<Ast::CallingConvDef::LoweringAction>(
                []
                {
                    return Ast::CallingConvDef::LoweringAction{ .m_kind = Ast::CallingConvDef::LoweringActionKind::Sret,
                                                                .m_regAssignKind =
                                                                        Ast::CallingConvDef::RegAssignKind::Sequence,
                                                                .m_registers = {},
                                                                .m_targetClass = std::nullopt,
                                                                .m_stackFallback = std::nullopt };
                });
    };

    static constexpr auto rule = []
    {
        auto seqBranch = dsl::peek(Common::Keyword<"REG_SEQ">::rule) >> dsl::p<ActionRegSeq>;
        auto slotsBranch = dsl::peek(Common::Keyword<"REG_SLOTS">::rule) >> dsl::p<ActionRegSlots>;
        auto expandBranch = dsl::peek(Common::Keyword<"EXPAND_TO">::rule) >> dsl::p<ActionExpandTo>;
        auto passPtrBranch = dsl::peek(Common::Keyword<"PASS_AS_POINTER">::rule) >> dsl::p<ActionPassAsPointer>;
        auto stackBranch = dsl::peek(Common::Keyword<"STACK">::rule) >> dsl::p<ActionStack>;
        auto sretBranch = dsl::peek(Common::Keyword<"SRET">::rule) >> dsl::p<ActionSret>;
        return seqBranch | slotsBranch | expandBranch | passPtrBranch | stackBranch | sretBranch;
    }();

    static constexpr auto value = lexy::forward<Ast::CallingConvDef::LoweringAction>;
};

struct DispatchRule
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + LEXY_LIT(">>") + dsl::p<LoweringAction> + dsl::lit_c<';'>;

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::DispatchRule>(
            [](Ast::Common::Identifier abiClass, Ast::CallingConvDef::LoweringAction action) {
                return Ast::CallingConvDef::DispatchRule{ .m_abiClass = std::move(abiClass),
                                                          .m_action = std::move(action) };
            });
};

struct SretConfig
{
    static constexpr auto whitespace = Common::Whitespace;

    using SretItem =
            std::variant<Ast::CallingConvDef::RegisterRef, bool, std::optional<Ast::CallingConvDef::RegisterRef>>;

    struct PassInRegDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"PASS_IN_REG">::rule >>
                (dsl::parenthesized(dsl::p<RegisterRef>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::forward<Ast::CallingConvDef::RegisterRef>;
    };

    struct ConsumesSlotDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"CONSUMES_ARG_SLOT">::rule >>
                (dsl::parenthesized(dsl::p<BooleanLit>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::forward<bool>;
    };

    struct ReturnRegDecl
    {
        static constexpr auto whitespace = Common::Whitespace;

        struct ReturnTarget
        {
            static constexpr auto whitespace = Common::Whitespace;
            static constexpr auto rule = []
            {
                auto noneBranch = dsl::peek(Common::Keyword<"NONE">::rule) >> Common::Keyword<"NONE">::rule;
                auto regBranch = dsl::else_ >> dsl::p<RegisterRef>;
                return noneBranch | regBranch;
            }();

            static constexpr auto value = lexy::callback<std::optional<Ast::CallingConvDef::RegisterRef>>(
                    [](Ast::CallingConvDef::RegisterRef reg) { return reg; }, [](auto...) { return std::nullopt; });
        };

        static constexpr auto rule = Common::Keyword<"RETURN_REG">::rule >>
                (dsl::parenthesized(dsl::p<ReturnTarget>) + dsl::lit_c<';'>);

        static constexpr auto value = lexy::forward<std::optional<Ast::CallingConvDef::RegisterRef>>;
    };

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto passBranch = dsl::peek(Common::Keyword<"PASS_IN_REG">::rule) >> dsl::p<PassInRegDecl>;
            auto slotBranch = dsl::peek(Common::Keyword<"CONSUMES_ARG_SLOT">::rule) >> dsl::p<ConsumesSlotDecl>;
            auto retBranch = dsl::peek(Common::Keyword<"RETURN_REG">::rule) >> dsl::p<ReturnRegDecl>;
            return passBranch | slotBranch | retBranch;
        }();

        static constexpr auto value = lexy::construct<SretItem>;
    };

    struct ItemList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Entry>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<SretItem>>;
    };

    static constexpr auto rule = Common::Keyword<"SRET_CONFIG">::rule >> (dsl::p<ItemList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::SretConfig>(
            [](std::pmr::vector<SretItem> items, auto...)
            {
                Ast::CallingConvDef::SretConfig cfg;
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::CallingConvDef::RegisterRef>)
                                {
                                    cfg.m_passInReg = std::move(val);
                                }
                                else if constexpr (std::is_same_v<T, bool>)
                                {
                                    cfg.m_consumesArgSlot = val;
                                }
                                else if constexpr (std::is_same_v<T, std::optional<Ast::CallingConvDef::RegisterRef>>)
                                {
                                    cfg.m_returnReg = std::move(val);
                                }
                            },
                            item);
                }
                return cfg;
            });
};

struct PassBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"PASS">::rule >>
            (dsl::curly_bracketed.list(dsl::p<DispatchRule>) + dsl::opt(dsl::lit_c<';'>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::CallingConvDef::DispatchRule>> >>
            lexy::callback<std::pmr::vector<Ast::CallingConvDef::DispatchRule>>(
                                          [](std::pmr::vector<Ast::CallingConvDef::DispatchRule> rules, auto...)
                                          { return rules; });
};

struct ReturnBlock
{
    static constexpr auto whitespace = Common::Whitespace;

    using ReturnItem = std::variant<Ast::CallingConvDef::DispatchRule, Ast::CallingConvDef::SretConfig>;

    struct Entry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto sretBranch = dsl::peek(Common::Keyword<"SRET_CONFIG">::rule) >> dsl::p<SretConfig>;
            auto ruleBranch = dsl::else_ >> dsl::p<DispatchRule>;
            return sretBranch | ruleBranch;
        }();

        static constexpr auto value = lexy::construct<ReturnItem>;
    };

    struct ReturnPayload
    {
        std::pmr::vector<Ast::CallingConvDef::DispatchRule> m_rules;
        std::optional<Ast::CallingConvDef::SretConfig> m_sretConfig;
    };

    static constexpr auto rule = Common::Keyword<"RETURN">::rule >>
            (dsl::curly_bracketed.list(dsl::p<Entry>) + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value =
            Common::PmrAsList<std::pmr::vector<ReturnItem>> >>
            lexy::callback<ReturnPayload>(
                    [](std::pmr::vector<ReturnItem> items, auto...)
                    {
                        ReturnPayload payload;
                        for (auto &item : items)
                        {
                            if (std::holds_alternative<Ast::CallingConvDef::DispatchRule>(item))
                            {
                                payload.m_rules.push_back(std::get<Ast::CallingConvDef::DispatchRule>(std::move(item)));
                            }
                            else if (std::holds_alternative<Ast::CallingConvDef::SretConfig>(item))
                            {
                                payload.m_sretConfig = std::get<Ast::CallingConvDef::SretConfig>(std::move(item));
                            }
                        }
                        return payload;
                    });
};

// =============================================================================
// 3. TOP LEVEL DIRECTIVES
// =============================================================================

struct ShadowSpaceTag
{
    Ast::Common::IntegerLiteral val;
};
struct StackPointerTag
{
    Ast::CallingConvDef::RegisterRef val;
};
struct FramePointerTag
{
    Ast::CallingConvDef::RegisterRef val;
};
struct CalleeSavedTag
{
    std::pmr::vector<Ast::CallingConvDef::RegisterRef> val;
};
struct CallerSavedTag
{
    std::pmr::vector<Ast::CallingConvDef::RegisterRef> val;
};
struct PassBlockTag
{
    std::pmr::vector<Ast::CallingConvDef::DispatchRule> val;
};

namespace Directives
{
struct StackAlignDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"STACK_ALIGN">::rule >>
            (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::Common::IntegerLiteral>;
};

struct StackDirectionDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"STACK_DIRECTION">::rule >>
            (dsl::parenthesized(dsl::p<StackDirection>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackDirection>;
};

struct StackCleanerDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"STACK_CLEANUP">::rule >>
            (dsl::parenthesized(dsl::p<StackCleaner>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::forward<Ast::CallingConvDef::StackCleaner>;
};

struct ShadowSpaceDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"SHADOW_SPACE">::rule >>
            (dsl::parenthesized(dsl::p<Common::IntegerLiteral>) + dsl::lit_c<';'>);
    static constexpr auto value =
            lexy::callback<ShadowSpaceTag>([](Ast::Common::IntegerLiteral lit) { return ShadowSpaceTag{ lit }; });
};

struct StackPointerDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"STACK_POINTER">::rule >>
            (dsl::parenthesized(dsl::p<RegisterRef>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::callback<StackPointerTag>([](Ast::CallingConvDef::RegisterRef reg)
                                                                  { return StackPointerTag{ std::move(reg) }; });
};

struct FramePointerDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"FRAME_POINTER">::rule >>
            (dsl::parenthesized(dsl::p<RegisterRef>) + dsl::lit_c<';'>);
    static constexpr auto value = lexy::callback<FramePointerTag>([](Ast::CallingConvDef::RegisterRef reg)
                                                                  { return FramePointerTag{ std::move(reg) }; });
};

struct CalleeSavedDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"CALLEE_SAVED">::rule >> (dsl::p<RegRefList> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::callback<CalleeSavedTag>(
            [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs) { return CalleeSavedTag{ std::move(regs) }; });
};

struct CallerSavedDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"CALLER_SAVED">::rule >> (dsl::p<RegRefList> + dsl::lit_c<';'>);
    static constexpr auto value = lexy::callback<CallerSavedTag>(
            [](std::pmr::vector<Ast::CallingConvDef::RegisterRef> regs) { return CallerSavedTag{ std::move(regs) }; });
};

struct PassBlockDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<PassBlock>;
    static constexpr auto value = lexy::callback<PassBlockTag>(
            [](std::pmr::vector<Ast::CallingConvDef::DispatchRule> rules) { return PassBlockTag{ std::move(rules) }; });
};
} // namespace Directives

struct CallingConvDefFile
{
    static constexpr auto whitespace = Common::Whitespace;

    using DirectiveItem = std::variant<Ast::Common::IntegerLiteral,
                                       Ast::CallingConvDef::StackDirection,
                                       Ast::CallingConvDef::StackCleaner,
                                       ShadowSpaceTag,
                                       StackPointerTag,
                                       FramePointerTag,
                                       CalleeSavedTag,
                                       CallerSavedTag,
                                       Ast::CallingConvDef::ClassifyBlock,
                                       PassBlockTag,
                                       ReturnBlock::ReturnPayload>;

    struct DirectiveEntry
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = []
        {
            auto stackAlign = dsl::peek(Common::Keyword<"STACK_ALIGN">::rule) >> dsl::p<Directives::StackAlignDecl>;
            auto stackDir =
                    dsl::peek(Common::Keyword<"STACK_DIRECTION">::rule) >> dsl::p<Directives::StackDirectionDecl>;
            auto stackCleanup =
                    dsl::peek(Common::Keyword<"STACK_CLEANUP">::rule) >> dsl::p<Directives::StackCleanerDecl>;
            auto shadowSpace = dsl::peek(Common::Keyword<"SHADOW_SPACE">::rule) >> dsl::p<Directives::ShadowSpaceDecl>;
            auto stackPtr = dsl::peek(Common::Keyword<"STACK_POINTER">::rule) >> dsl::p<Directives::StackPointerDecl>;
            auto framePtr = dsl::peek(Common::Keyword<"FRAME_POINTER">::rule) >> dsl::p<Directives::FramePointerDecl>;
            auto calleeSaved = dsl::peek(Common::Keyword<"CALLEE_SAVED">::rule) >> dsl::p<Directives::CalleeSavedDecl>;
            auto callerSaved = dsl::peek(Common::Keyword<"CALLER_SAVED">::rule) >> dsl::p<Directives::CallerSavedDecl>;
            auto classify = dsl::peek(Common::Keyword<"CLASSIFY">::rule) >> dsl::p<ClassifyBlock>;
            auto pass = dsl::peek(Common::Keyword<"PASS">::rule) >> dsl::p<Directives::PassBlockDecl>;
            auto ret = dsl::peek(Common::Keyword<"RETURN">::rule) >> dsl::p<ReturnBlock>;

            return stackAlign | stackDir | stackCleanup | shadowSpace | stackPtr | framePtr | calleeSaved |
                    callerSaved | classify | pass | ret;
        }();

        static constexpr auto value = lexy::construct<DirectiveItem>;
    };

    struct DirectiveList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<DirectiveEntry>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<DirectiveItem>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof).opt(
            Common::Keyword<"calling_conv">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<DirectiveList> + dsl::opt(dsl::lit_c<';'>)));

    static constexpr auto value = lexy::callback<Ast::CallingConvDef::CallingConvDefFile>(
            [](Ast::Common::Identifier name, std::pmr::vector<DirectiveItem> items, auto...)
            {
                Ast::CallingConvDef::CallingConvDefFile def;
                def.m_name = std::move(name);

                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::Common::IntegerLiteral>)
                                {
                                    def.m_stackAlign = val;
                                }
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::StackDirection>)
                                {
                                    def.m_stackDirection = val;
                                }
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::StackCleaner>)
                                {
                                    def.m_stackCleanup = val;
                                }
                                else if constexpr (std::is_same_v<T, ShadowSpaceTag>)
                                {
                                    def.m_shadowSpace = val.val;
                                }
                                else if constexpr (std::is_same_v<T, StackPointerTag>)
                                {
                                    def.m_stackPointer = std::move(val.val);
                                }
                                else if constexpr (std::is_same_v<T, FramePointerTag>)
                                {
                                    def.m_framePointer = std::move(val.val);
                                }
                                else if constexpr (std::is_same_v<T, CalleeSavedTag>)
                                {
                                    def.m_calleeSaved = std::move(val.val);
                                }
                                else if constexpr (std::is_same_v<T, CallerSavedTag>)
                                {
                                    def.m_callerSaved = std::move(val.val);
                                }
                                else if constexpr (std::is_same_v<T, Ast::CallingConvDef::ClassifyBlock>)
                                {
                                    def.m_classify = std::move(val);
                                }
                                else if constexpr (std::is_same_v<T, PassBlockTag>)
                                {
                                    def.m_passRules = std::move(val.val);
                                }
                                else if constexpr (std::is_same_v<T, ReturnBlock::ReturnPayload>)
                                {
                                    def.m_returnRules = std::move(val.m_rules);
                                    def.m_sretConfig = std::move(val.m_sretConfig);
                                }
                            },
                            item);
                }
                return def;
            },
            [](auto...) { return Ast::CallingConvDef::CallingConvDefFile{}; });
};

} // namespace DSL::Parser::CallingConvDef

#endif // EZDSL_CALLING_CONV_DEF_LANG_H