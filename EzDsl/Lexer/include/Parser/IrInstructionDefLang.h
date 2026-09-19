#ifndef EZDSL_PARSER_IR_INST_DEF_LANG_H
#define EZDSL_PARSER_IR_INST_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::IrInstDef
{
namespace dsl = ::lexy::dsl;

/**
 * Parses an operand-type keyword (Register, Integer, ...) into Ast::IrInstDef::IrOperandType.
 */
struct OperandType
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::IrInstDef::IrOperandType>
            .map(LEXY_LIT("Register"), Ast::IrInstDef::IrOperandType::Register)
            .map(LEXY_LIT("Integer"), Ast::IrInstDef::IrOperandType::Integer)
            .map(LEXY_LIT("FloatingPoint"), Ast::IrInstDef::IrOperandType::FloatingPoint)
            .map(LEXY_LIT("Memory"), Ast::IrInstDef::IrOperandType::Memory)
            .map(LEXY_LIT("Reference"), Ast::IrInstDef::IrOperandType::Reference)
            .map(LEXY_LIT("RuntimeSymbol"), Ast::IrInstDef::IrOperandType::RuntimeSymbol)
            .map(LEXY_LIT("VariadicArgs"), Ast::IrInstDef::IrOperandType::VariadicArgs)
            .map(LEXY_LIT("Immediate"), Ast::IrInstDef::IrOperandType::Immediate)
            .map(LEXY_LIT("RegIntImm"), Ast::IrInstDef::IrOperandType::RegIntImm)
            .map(LEXY_LIT("RegFloatImm"), Ast::IrInstDef::IrOperandType::RegFloatImm)
            .map(LEXY_LIT("RegImm"), Ast::IrInstDef::IrOperandType::RegImm)
            .map(LEXY_LIT("AddressSource"), Ast::IrInstDef::IrOperandType::AddressSource)
            .map(LEXY_LIT("AnyValue"), Ast::IrInstDef::IrOperandType::AnyValue)
            .map(LEXY_LIT("Any"), Ast::IrInstDef::IrOperandType::Any);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::IrInstDef::IrOperandType>;
};

/**
 * Parses an operand direction keyword (IN/OUT/INOUT) into Ast::IrInstDef::IrOperandDir.
 */
struct Direction
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::IrInstDef::IrOperandDir>
            .map(LEXY_LIT("IN"), Ast::IrInstDef::IrOperandDir::ArgIn)
            .map(LEXY_LIT("OUT"), Ast::IrInstDef::IrOperandDir::ArgOut)
            .map(LEXY_LIT("INOUT"), Ast::IrInstDef::IrOperandDir::ArgInOut);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::IrInstDef::IrOperandDir>;
};

/**
 * Parses `Type:name DIR` (e.g. `Integer:imm IN`) and produces an IrOperand.
 */
struct IrOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<OperandType> + dsl::lit_c<':'> + dsl::p<Common::Identifier> + dsl::p<Direction>;

    static constexpr auto value = lexy::callback<Ast::IrInstDef::IrOperand>(
            [](Ast::IrInstDef::IrOperandType type, Ast::Common::Identifier name, Ast::IrInstDef::IrOperandDir dir)
            { return Ast::IrInstDef::IrOperand{ .m_type = type, .m_name = std::move(name), .m_dir = dir }; });
};

/**
 * Parses an instruction category keyword (DataMovement, Memory, ...) into IrInstCategory.
 */
struct Category
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::IrInstDef::IrInstCategory>
            .map(LEXY_LIT("DataMovement"), Ast::IrInstDef::IrInstCategory::DataMovement)
            .map(LEXY_LIT("Memory"), Ast::IrInstDef::IrInstCategory::Memory)
            .map(LEXY_LIT("Arithmetic"), Ast::IrInstDef::IrInstCategory::Arithmetic)
            .map(LEXY_LIT("Bitwise"), Ast::IrInstDef::IrInstCategory::Bitwise)
            .map(LEXY_LIT("Compare"), Ast::IrInstDef::IrInstCategory::Compare)
            .map(LEXY_LIT("ControlFlow"), Ast::IrInstDef::IrInstCategory::ControlFlow)
            .map(LEXY_LIT("Casting"), Ast::IrInstDef::IrInstCategory::Casting)
            .map(LEXY_LIT("System"), Ast::IrInstDef::IrInstCategory::System);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::IrInstDef::IrInstCategory>;
};

/**
 * Parses an instruction tier keyword (HighLevel, PassInternal, TargetLow) into IrInstTier.
 */
struct Tier
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::IrInstDef::IrInstTier>
            .map(LEXY_LIT("HighLevel"), Ast::IrInstDef::IrInstTier::HighLevel)
            .map(LEXY_LIT("PassInternal"), Ast::IrInstDef::IrInstTier::PassInternal)
            .map(LEXY_LIT("TargetLow"), Ast::IrInstDef::IrInstTier::TargetLow);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::IrInstDef::IrInstTier>;
};

/**
 * Parses a behavioral flag keyword (SizeMatch, ReadsMemory, ...) into IrInstFlag.
 */
struct InstFlag
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::IrInstDef::IrInstFlag>
            .map(LEXY_LIT("SizeMatch"), Ast::IrInstDef::IrInstFlag::SizeMatch)
            .map(LEXY_LIT("DestLarger"), Ast::IrInstDef::IrInstFlag::DestLarger)
            .map(LEXY_LIT("DestSmaller"), Ast::IrInstDef::IrInstFlag::DestSmaller)
            .map(LEXY_LIT("ReadsMemory"), Ast::IrInstDef::IrInstFlag::ReadsMemory)
            .map(LEXY_LIT("WritesMemory"), Ast::IrInstDef::IrInstFlag::WritesMemory)
            .map(LEXY_LIT("IsTerminator"), Ast::IrInstDef::IrInstFlag::IsTerminator)
            .map(LEXY_LIT("IsBranch"), Ast::IrInstDef::IrInstFlag::IsBranch)
            .map(LEXY_LIT("IsCall"), Ast::IrInstDef::IrInstFlag::IsCall)
            .map(LEXY_LIT("IsReturn"), Ast::IrInstDef::IrInstFlag::IsReturn)
            .map(LEXY_LIT("HasSideEffect"), Ast::IrInstDef::IrInstFlag::HasSideEffect)
            .map(LEXY_LIT("IsCommutative"), Ast::IrInstDef::IrInstFlag::IsCommutative)
            .map(LEXY_LIT("ReadsCPUFlags"), Ast::IrInstDef::IrInstFlag::ReadsCPUFlags)
            .map(LEXY_LIT("WritesCPUFlags"), Ast::IrInstDef::IrInstFlag::WritesCPUFlags)
            .map(LEXY_LIT("TreatAsSigned"), Ast::IrInstDef::IrInstFlag::TreatAsSigned)
            .map(LEXY_LIT("VariadicArgs"), Ast::IrInstDef::IrInstFlag::VariadicArgs);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::IrInstDef::IrInstFlag>;
};

/**
 * One statement inside an `ir_inst { ... }` body: CATEGORY, TIER, or FLAGS, exposed as a variant.
 */
struct BodyItem
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses `CATEGORY(Kind);` and forwards the IrInstCategory.
     */
    struct CategoryDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"CATEGORY">::rule >>
                (dsl::parenthesized(dsl::p<Category>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::forward<Ast::IrInstDef::IrInstCategory>;
    };

    /**
     * Parses `TIER(Tier);` and forwards the IrInstTier.
     */
    struct TierDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"TIER">::rule >>
                (dsl::parenthesized(dsl::p<Tier>) + dsl::lit_c<';'>);
        static constexpr auto value = lexy::forward<Ast::IrInstDef::IrInstTier>;
    };

    /**
     * Parses `FLAGS(A, B, ...);` into a PMR vector of IrInstFlag.
     */
    struct FlagsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"FLAGS">::rule >>
                (dsl::parenthesized.list(dsl::p<InstFlag>, dsl::sep(dsl::lit_c<','>)) + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::IrInstDef::IrInstFlag>>;
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"CATEGORY">::rule) >> dsl::p<CategoryDecl>) |
            (dsl::peek(Common::Keyword<"TIER">::rule) >> dsl::p<TierDecl>) |
            (dsl::peek(Common::Keyword<"FLAGS">::rule) >> dsl::p<FlagsDecl>);

    static constexpr auto value = lexy::construct<std::variant<Ast::IrInstDef::IrInstCategory,
                                                               Ast::IrInstDef::IrInstTier,
                                                               std::pmr::vector<Ast::IrInstDef::IrInstFlag>>>;
};

/**
 * Parses one `ir_inst name(operands) { body }` declaration and assembles an IrInstDecl.
 */
struct IrInstDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses a `,`-separated list of IrOperands into a PMR vector.
     */
    struct NonEmptyOperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<IrOperand>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::IrInstDef::IrOperand>>;
    };

    /**
     * Parses the parenthesized operand signature, allowing an empty `()` list.
     */
    struct OperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized(
                dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<NonEmptyOperandList>));
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::IrInstDef::IrOperand>>(
                [](std::pmr::vector<Ast::IrInstDef::IrOperand> list) { return list; },
                [](lexy::nullopt) { return std::pmr::vector<Ast::IrInstDef::IrOperand>{}; });
    };

    /**
     * Parses the curly-braced body as a list of BodyItem variants.
     */
    struct BodyList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<BodyItem>);
        static constexpr auto value =
                Common::PmrAsList<std::pmr::vector<std::variant<Ast::IrInstDef::IrInstCategory,
                                                                Ast::IrInstDef::IrInstTier,
                                                                std::pmr::vector<Ast::IrInstDef::IrInstFlag>>>>;
    };

    static constexpr auto rule = Common::Keyword<"ir_inst">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<OperandList> + dsl::p<BodyList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::IrInstDef::IrInstDecl>(
            [](Ast::Common::Identifier name,
               std::pmr::vector<Ast::IrInstDef::IrOperand> operands,
               auto bodyItems,
               auto...)
            {
                Ast::IrInstDef::IrInstDecl decl;
                decl.m_name = std::move(name);
                decl.m_operands = std::move(operands);

                for (auto &item : bodyItems)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, Ast::IrInstDef::IrInstCategory>)
                                    decl.m_body.m_category = val;
                                else if constexpr (std::is_same_v<T, Ast::IrInstDef::IrInstTier>)
                                    decl.m_body.m_tier = val;
                                else if constexpr (std::is_same_v<T, std::pmr::vector<Ast::IrInstDef::IrInstFlag>>)
                                    decl.m_body.m_flags = std::move(val);
                            },
                            item);
                }
                return decl;
            });
};

/**
 * Parses a whole `.irdf` file as an EOF-terminated list of IrInstDecls into an IrInstDefFile.
 */
struct IrInstDefFile
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<IrInstDecl>);
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::IrInstDef::IrInstDecl>> >>
            lexy::construct<Ast::IrInstDef::IrInstDefFile>;
};

} // namespace DSL::Parser::IrInstDef

#endif // EZDSL_PARSER_IR_INST_DEF_LANG_H