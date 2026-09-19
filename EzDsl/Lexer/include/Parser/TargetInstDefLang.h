#ifndef EZDSL_PARSER_TARGET_INST_DEF_LANG_H
#define EZDSL_PARSER_TARGET_INST_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/TargetInstDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TargetInstDef
{
namespace dsl = ::lexy::dsl;

/**
 * Parses an operand direction keyword (IN/OUT/INOUT) into OperandDirection.
 */
struct Direction
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::TargetInstDef::OperandDirection>
            .map(LEXY_LIT("IN"), Ast::TargetInstDef::OperandDirection::In)
            .map(LEXY_LIT("OUT"), Ast::TargetInstDef::OperandDirection::Out)
            .map(LEXY_LIT("INOUT"), Ast::TargetInstDef::OperandDirection::InOut);

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha));
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::OperandDirection>;
};

/**
 * Parses `RegClassOrType:name DIR` into a TargetOperandDecl.
 */
struct TargetOperand
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier> + dsl::p<Direction>;

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetOperandDecl>(
            [](Ast::Common::Identifier regClassOrType,
               Ast::Common::Identifier name,
               Ast::TargetInstDef::OperandDirection dir)
            {
                return Ast::TargetInstDef::TargetOperandDecl{ .m_regClassOrType = std::move(regClassOrType),
                                                              .m_name = std::move(name),
                                                              .m_direction = dir };
            });
};

// ============================================================================
// ENCODING { ... } block
// ============================================================================

/**
 * Parses a single raw byte literal (decimal or 0x-prefixed hex).
 */
struct ByteLiteral
{
    static constexpr auto rule = []
    {
        auto hex = (dsl::lit<"0x"> | dsl::lit<"0X">) >> dsl::integer<uint8_t, dsl::hex>(dsl::digits<dsl::hex>);
        auto dec = dsl::integer<uint8_t>(dsl::digits<dsl::decimal>);
        return hex | dec;
    }();
    static constexpr auto value = lexy::forward<uint8_t>;
};

/**
 * Parses a `[ 0x0F, 0xB6 ]` list of raw opcode bytes into a PMR vector<uint8_t>.
 */
struct ByteList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::square_bracketed.list(dsl::p<ByteLiteral>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<std::pmr::vector<uint8_t>>;
};

/**
 * Parses an encoding-form keyword (rr, rm, movzx, jcc, ...) into EncForm.
 */
struct EncFormSymbol
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::TargetInstDef::EncForm>
            .map(LEXY_LIT("rr"), Ast::TargetInstDef::EncForm::Rr)
            .map(LEXY_LIT("rm"), Ast::TargetInstDef::EncForm::Rm)
            .map(LEXY_LIT("mr"), Ast::TargetInstDef::EncForm::Mr)
            .map(LEXY_LIT("ri"), Ast::TargetInstDef::EncForm::Ri)
            .map(LEXY_LIT("movri"), Ast::TargetInstDef::EncForm::MovRI)
            .map(LEXY_LIT("movzx"), Ast::TargetInstDef::EncForm::Movzx)
            .map(LEXY_LIT("movsx"), Ast::TargetInstDef::EncForm::Movsx)
            .map(LEXY_LIT("lea"), Ast::TargetInstDef::EncForm::Lea)
            .map(LEXY_LIT("unary"), Ast::TargetInstDef::EncForm::Unary)
            .map(LEXY_LIT("test"), Ast::TargetInstDef::EncForm::Test)
            .map(LEXY_LIT("shift"), Ast::TargetInstDef::EncForm::Shift)
            .map(LEXY_LIT("imul_rr"), Ast::TargetInstDef::EncForm::ImulRR)
            .map(LEXY_LIT("imul_ri"), Ast::TargetInstDef::EncForm::ImulRI)
            .map(LEXY_LIT("div"), Ast::TargetInstDef::EncForm::Div)
            .map(LEXY_LIT("jcc"), Ast::TargetInstDef::EncForm::Jcc)
            .map(LEXY_LIT("jmp"), Ast::TargetInstDef::EncForm::Jmp)
            .map(LEXY_LIT("call"), Ast::TargetInstDef::EncForm::Call)
            .map(LEXY_LIT("ret"), Ast::TargetInstDef::EncForm::Ret)
            .map(LEXY_LIT("push"), Ast::TargetInstDef::EncForm::Push)
            .map(LEXY_LIT("pop"), Ast::TargetInstDef::EncForm::Pop)
            .map(LEXY_LIT("nop"), Ast::TargetInstDef::EncForm::Nop)
            .map(LEXY_LIT("syscall"), Ast::TargetInstDef::EncForm::Syscall)
            .map(LEXY_LIT("setcc"), Ast::TargetInstDef::EncForm::Setcc)
            .map(LEXY_LIT("sse"), Ast::TargetInstDef::EncForm::Sse)
            .map(LEXY_LIT("cvt"), Ast::TargetInstDef::EncForm::Cvt);

    static constexpr auto rule =
            dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore));
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::EncForm>;
};

/**
 * Parses an encoding-slot keyword (reg, rm_reg, imm32, rel32, ...) into EncSlotKind.
 */
struct EncSlotSymbol
{
    static constexpr auto Table =
        lexy::symbol_table<Ast::TargetInstDef::EncSlotKind>
            .map(LEXY_LIT("reg"), Ast::TargetInstDef::EncSlotKind::Reg)
            .map(LEXY_LIT("rm_reg"), Ast::TargetInstDef::EncSlotKind::RmReg)
            .map(LEXY_LIT("rm_mem"), Ast::TargetInstDef::EncSlotKind::RmMem)
            .map(LEXY_LIT("imm8"), Ast::TargetInstDef::EncSlotKind::Imm8)
            .map(LEXY_LIT("imm16"), Ast::TargetInstDef::EncSlotKind::Imm16)
            .map(LEXY_LIT("imm32"), Ast::TargetInstDef::EncSlotKind::Imm32)
            .map(LEXY_LIT("imm64"), Ast::TargetInstDef::EncSlotKind::Imm64)
            .map(LEXY_LIT("imm8_signed"), Ast::TargetInstDef::EncSlotKind::Imm8Signed)
            .map(LEXY_LIT("rel8"), Ast::TargetInstDef::EncSlotKind::Rel8)
            .map(LEXY_LIT("rel32"), Ast::TargetInstDef::EncSlotKind::Rel32)
            .map(LEXY_LIT("cc"), Ast::TargetInstDef::EncSlotKind::CondCode);

    static constexpr auto rule =
            dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_underscore, dsl::ascii::alpha_digit_underscore));
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::EncSlotKind>;
};

/**
 * Parses an x86 prefix keyword (P66, F2, ...) into its one-bit prefix mask value.
 */
struct PrefixSymbol
{
    static constexpr auto Table =
        lexy::symbol_table<uint8_t>
            .map(LEXY_LIT("P66"), static_cast<uint8_t>(1u << 0))
            .map(LEXY_LIT("P67"), static_cast<uint8_t>(1u << 1))
            .map(LEXY_LIT("F2"), static_cast<uint8_t>(1u << 2))
            .map(LEXY_LIT("F3"), static_cast<uint8_t>(1u << 3))
            .map(LEXY_LIT("F0"), static_cast<uint8_t>(1u << 4));

    static constexpr auto rule = dsl::symbol<Table>(dsl::identifier(dsl::ascii::alpha_digit_underscore));
    static constexpr auto value = lexy::forward<uint8_t>;
};

/**
 * Parses a `name => slot` operand binding inside an ENCODING operands block.
 */
struct EncOperandBindingParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + LEXY_LIT("=>") + dsl::p<EncSlotSymbol>;
    static constexpr auto value = lexy::callback<Ast::TargetInstDef::EncOperandBinding>(
            [](Ast::Common::Identifier name, Ast::TargetInstDef::EncSlotKind slot)
            { return Ast::TargetInstDef::EncOperandBinding{ .m_name = std::move(name), .m_slot = slot }; });
};

/**
 * Parses an `operands { name => slot; ... }` block into a PMR list of EncOperandBinding.
 */
struct EncOperandsBlock
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"operands">::rule >>
            dsl::curly_bracketed.list(dsl::p<EncOperandBindingParser> + dsl::lit_c<';'>);
    static constexpr auto value = Common::PmrAsList<Ast::TargetInstDef::EncOperandBinding>;
};

/**
 * Parses an `ENCODING { ... }` block, collecting each field variant and folding it into an
 * EncodingDecl.
 */
struct EncodingDeclParser
{
    static constexpr auto whitespace = Common::Whitespace;

    // Tag structs wrap each parsed ENCODING field so the ItemVariant can distinguish them.
    struct TagForm
    {
        Ast::TargetInstDef::EncForm val; // Encoding form.
    };
    struct TagOpcode
    {
        std::pmr::vector<uint8_t> val; // Primary opcode bytes.
    };
    struct TagOpcodeDigit
    {
        uint8_t val; // ModRM /digit field.
    };
    struct TagRexW
    {
        bool val; // Always-set REX.W.
    };
    struct TagRexWBySize
    {
        bool val; // REX.W selected from the size operand.
    };
    struct TagPrefixes
    {
        uint8_t val; // Legacy prefix bitmask.
    };
    struct TagOperands
    {
        std::pmr::vector<Ast::TargetInstDef::EncOperandBinding> val; // Operand-to-slot bindings.
    };
    struct TagCoalesce
    {
        Ast::Common::Identifier val; // Two-address source operand name.
    };
    struct TagSize
    {
        Ast::Common::Identifier val; // Operand determining operation size.
    };
    struct TagShiftCl
    {
        bool val; // Shift amount fixed to CL.
    };
    struct TagByteRex
    {
        bool val; // Force REX on byte forms.
    };
    struct TagCond
    {
        uint8_t val; // Condition-code digit.
    };
    struct TagSsePrefix
    {
        uint8_t val; // SSE prefix bitmask.
    };
    struct TagSseOpcode
    {
        std::pmr::vector<uint8_t> val; // SSE opcode bytes.
    };

    using ItemVariant = std::variant<TagForm,
                                     TagOpcode,
                                     TagOpcodeDigit,
                                     TagRexW,
                                     TagRexWBySize,
                                     TagPrefixes,
                                     TagOperands,
                                     TagCoalesce,
                                     TagSize,
                                     TagShiftCl,
                                     TagByteRex,
                                     TagCond,
                                     TagSsePrefix,
                                     TagSseOpcode>;

    /**
     * Parses `form: FORM` into a TagForm.
     */
    struct FormItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"form">::rule >> dsl::lit_c<':'> >> dsl::p<EncFormSymbol>;
        static constexpr auto value =
                lexy::callback<TagForm>([](Ast::TargetInstDef::EncForm f) { return TagForm{ f }; });
    };

    /**
     * Parses `opcode: [...]` into a TagOpcode.
     */
    struct OpcodeItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"opcode">::rule >> dsl::lit_c<':'> >> dsl::p<ByteList>;
        static constexpr auto value = lexy::callback<TagOpcode>([](std::pmr::vector<uint8_t> bytes)
                                                                { return TagOpcode{ std::move(bytes) }; });
    };

    /**
     * Parses `opcode_digit: N` into a TagOpcodeDigit.
     */
    struct OpcodeDigitItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                Common::Keyword<"opcode_digit">::rule >> dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>;
        static constexpr auto value = lexy::callback<TagOpcodeDigit>(
                [](Ast::Common::IntegerLiteral v) { return TagOpcodeDigit{ static_cast<uint8_t>(v.m_node) }; });
    };

    /**
     * Parses `rex_w: bool` into a TagRexW.
     */
    struct RexWItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                Common::Keyword<"rex_w">::rule >> dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>;
        static constexpr auto value =
                lexy::callback<TagRexW>([](Ast::Common::BooleanLiteral v) { return TagRexW{ v.m_node }; });
    };

    /**
     * Parses `rex_w_size: bool` into a TagRexWBySize.
     */
    struct RexWBySizeItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                Common::Keyword<"rex_w_size">::rule >> dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>;
        static constexpr auto value =
                lexy::callback<TagRexWBySize>([](Ast::Common::BooleanLiteral v) { return TagRexWBySize{ v.m_node }; });
    };

    /**
     * Parses `prefixes: PREFIX` into a TagPrefixes.
     */
    struct PrefixesItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"prefixes">::rule >> dsl::lit_c<':'> >> dsl::p<PrefixSymbol>;
        static constexpr auto value = lexy::callback<TagPrefixes>([](uint8_t v) { return TagPrefixes{ v }; });
    };

    /**
     * Parses an `operands { ... }` block into a TagOperands.
     */
    struct OperandsItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<EncOperandsBlock>;
        static constexpr auto value =
                lexy::callback<TagOperands>([](std::pmr::vector<Ast::TargetInstDef::EncOperandBinding> ops)
                                            { return TagOperands{ std::move(ops) }; });
    };

    /**
     * Parses `coalesce: OPERAND` into a TagCoalesce.
     */
    struct CoalesceItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"coalesce">::rule >> dsl::lit_c<':'> >> dsl::p<Common::Identifier>;
        static constexpr auto value =
                lexy::callback<TagCoalesce>([](Ast::Common::Identifier id) { return TagCoalesce{ std::move(id) }; });
    };

    /**
     * Parses `size: OPERAND` into a TagSize.
     */
    struct SizeItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"size">::rule >> dsl::lit_c<':'> >> dsl::p<Common::Identifier>;
        static constexpr auto value =
                lexy::callback<TagSize>([](Ast::Common::Identifier id) { return TagSize{ std::move(id) }; });
    };

    /**
     * Parses `shift_cl: bool` into a TagShiftCl.
     */
    struct ShiftClItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                Common::Keyword<"shift_cl">::rule >> dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>;
        static constexpr auto value =
                lexy::callback<TagShiftCl>([](Ast::Common::BooleanLiteral v) { return TagShiftCl{ v.m_node }; });
    };

    /**
     * Parses `byte_rex: bool` into a TagByteRex.
     */
    struct ByteRexItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                Common::Keyword<"byte_rex">::rule >> dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>;
        static constexpr auto value =
                lexy::callback<TagByteRex>([](Ast::Common::BooleanLiteral v) { return TagByteRex{ v.m_node }; });
    };

    /**
     * Parses `cond: N` into a TagCond.
     */
    struct CondItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"cond">::rule >> dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>;
        static constexpr auto value = lexy::callback<TagCond>([](Ast::Common::IntegerLiteral v)
                                                              { return TagCond{ static_cast<uint8_t>(v.m_node) }; });
    };

    /**
     * Parses `sse_prefix: PREFIX` into a TagSsePrefix.
     */
    struct SsePrefixItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"sse_prefix">::rule >> dsl::lit_c<':'> >> dsl::p<PrefixSymbol>;
        static constexpr auto value = lexy::callback<TagSsePrefix>([](uint8_t v) { return TagSsePrefix{ v }; });
    };

    /**
     * Parses `sse_opcode: [...]` into a TagSseOpcode.
     */
    struct SseOpcodeItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"sse_opcode">::rule >> dsl::lit_c<':'> >> dsl::p<ByteList>;
        static constexpr auto value = lexy::callback<TagSseOpcode>([](std::pmr::vector<uint8_t> bytes)
                                                                   { return TagSseOpcode{ std::move(bytes) }; });
    };

    /**
     * Dispatches one ENCODING field keyword to its item parser.
     */
    struct Item
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = (dsl::peek(Common::Keyword<"form">::rule) >> dsl::p<FormItem>) |
                (dsl::peek(Common::Keyword<"opcode">::rule) >> dsl::p<OpcodeItem>) |
                (dsl::peek(Common::Keyword<"opcode_digit">::rule) >> dsl::p<OpcodeDigitItem>) |
                (dsl::peek(Common::Keyword<"rex_w_size">::rule) >> dsl::p<RexWBySizeItem>) |
                (dsl::peek(Common::Keyword<"rex_w">::rule) >> dsl::p<RexWItem>) |
                (dsl::peek(Common::Keyword<"prefixes">::rule) >> dsl::p<PrefixesItem>) |
                (dsl::peek(Common::Keyword<"operands">::rule) >> dsl::p<OperandsItem>) |
                (dsl::peek(Common::Keyword<"coalesce">::rule) >> dsl::p<CoalesceItem>) |
                (dsl::peek(Common::Keyword<"size">::rule) >> dsl::p<SizeItem>) |
                (dsl::peek(Common::Keyword<"shift_cl">::rule) >> dsl::p<ShiftClItem>) |
                (dsl::peek(Common::Keyword<"byte_rex">::rule) >> dsl::p<ByteRexItem>) |
                (dsl::peek(Common::Keyword<"cond">::rule) >> dsl::p<CondItem>) |
                (dsl::peek(Common::Keyword<"sse_prefix">::rule) >> dsl::p<SsePrefixItem>) |
                (dsl::peek(Common::Keyword<"sse_opcode">::rule) >> dsl::p<SseOpcodeItem>);

        static constexpr auto value = lexy::forward<ItemVariant>;
    };

    /**
     * Parses the `;`-separated ENCODING fields into a PMR vector of item variants.
     */
    struct ItemList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<Item> + dsl::lit_c<';'>);
        static constexpr auto value = Common::PmrAsList<ItemVariant>;
    };

    static constexpr auto rule = Common::Keyword<"ENCODING">::rule >> (dsl::p<ItemList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::EncodingDecl>(
            [](std::pmr::vector<ItemVariant> items, auto...)
            {
                Ast::TargetInstDef::EncodingDecl decl;
                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, TagForm>)
                                    decl.m_form = val.val;
                                else if constexpr (std::is_same_v<T, TagOpcode>)
                                    decl.m_opcode = std::move(val.val);
                                else if constexpr (std::is_same_v<T, TagOpcodeDigit>)
                                    decl.m_opcodeDigit = val.val;
                                else if constexpr (std::is_same_v<T, TagRexW>)
                                    decl.m_rexW = val.val;
                                else if constexpr (std::is_same_v<T, TagRexWBySize>)
                                    decl.m_rexWBySize = val.val;
                                else if constexpr (std::is_same_v<T, TagPrefixes>)
                                    decl.m_prefixes = val.val;
                                else if constexpr (std::is_same_v<T, TagOperands>)
                                    decl.m_operands = std::move(val.val);
                                else if constexpr (std::is_same_v<T, TagCoalesce>)
                                    decl.m_coalesce = std::move(val.val);
                                else if constexpr (std::is_same_v<T, TagSize>)
                                    decl.m_sizeOperand = std::move(val.val);
                                else if constexpr (std::is_same_v<T, TagShiftCl>)
                                    decl.m_shiftByCL = val.val;
                                else if constexpr (std::is_same_v<T, TagByteRex>)
                                    decl.m_byteRex = val.val;
                                else if constexpr (std::is_same_v<T, TagCond>)
                                    decl.m_condCode = val.val;
                                else if constexpr (std::is_same_v<T, TagSsePrefix>)
                                {
                                    decl.m_ssePrefixes = val.val;
                                    decl.m_hasSseVariant = true;
                                }
                                else if constexpr (std::is_same_v<T, TagSseOpcode>)
                                {
                                    decl.m_sseOpcode = std::move(val.val);
                                    decl.m_hasSseVariant = true;
                                }
                            },
                            item);
                }
                return decl;
            });
};

/**
 * One statement inside a `target_inst { ... }` body (MNEMONIC, FLAGS, implicit effects, or
 * ENCODING), exposed as a variant.
 */
struct BodyItem
{
    static constexpr auto whitespace = Common::Whitespace;

    // Tag structs wrap each parsed body field so ItemVariant can distinguish them.
    struct TagMnemonic
    {
        Ast::Common::StringLiteral val; // Assembly mnemonic.
    };
    struct TagFlags
    {
        std::pmr::vector<Ast::Common::Identifier> val; // Behavioral flags.
    };
    struct TagImplicitDefs
    {
        std::pmr::vector<Ast::Common::Identifier> val; // Implicitly defined registers.
    };
    struct TagImplicitUses
    {
        std::pmr::vector<Ast::Common::Identifier> val; // Implicitly used registers.
    };
    struct TagEncoding
    {
        Ast::TargetInstDef::EncodingDecl val; // Machine encoding.
    };

    using ItemVariant = std::variant<TagMnemonic, TagFlags, TagImplicitDefs, TagImplicitUses, TagEncoding>;

    /**
     * Parses a `,`-separated identifier list into a PMR vector.
     */
    struct IdList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::Common::Identifier>>;
    };

    /**
     * Parses an optional parenthesized identifier list, yielding an empty vector when omitted.
     */
    struct OptIdList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                dsl::parenthesized(dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<IdList>));
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::Common::Identifier>>(
                [](std::pmr::vector<Ast::Common::Identifier> list) { return list; },
                [](lexy::nullopt) { return std::pmr::vector<Ast::Common::Identifier>{}; });
    };

    /**
     * Parses `MNEMONIC("text");` into a TagMnemonic.
     */
    struct MnemonicDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"MNEMONIC">::rule >>
                (dsl::parenthesized(dsl::p<Common::StringLiteral>) + dsl::lit_c<';'>);
        static constexpr auto value =
                lexy::callback<TagMnemonic>([](Ast::Common::StringLiteral s) { return TagMnemonic{ std::move(s) }; });
    };

    /**
     * Parses `FLAGS(...);` into a TagFlags.
     */
    struct FlagsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"FLAGS">::rule >> (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagFlags>([](std::pmr::vector<Ast::Common::Identifier> list)
                                                               { return TagFlags{ std::move(list) }; });
    };

    /**
     * Parses `IMPLICIT_DEFS(...);` into a TagImplicitDefs.
     */
    struct ImplicitDefsDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IMPLICIT_DEFS">::rule >> (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagImplicitDefs>([](std::pmr::vector<Ast::Common::Identifier> list)
                                                                      { return TagImplicitDefs{ std::move(list) }; });
    };

    /**
     * Parses `IMPLICIT_USES(...);` into a TagImplicitUses.
     */
    struct ImplicitUsesDecl
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = Common::Keyword<"IMPLICIT_USES">::rule >> (dsl::p<OptIdList> + dsl::lit_c<';'>);
        static constexpr auto value = lexy::callback<TagImplicitUses>([](std::pmr::vector<Ast::Common::Identifier> list)
                                                                      { return TagImplicitUses{ std::move(list) }; });
    };

    /**
     * Parses an `ENCODING { ... }` block into a TagEncoding.
     */
    struct EncodingDeclItem
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::p<EncodingDeclParser>;
        static constexpr auto value = lexy::callback<TagEncoding>([](Ast::TargetInstDef::EncodingDecl d)
                                                                  { return TagEncoding{ std::move(d) }; });
    };

    static constexpr auto rule = (dsl::peek(Common::Keyword<"MNEMONIC">::rule) >> dsl::p<MnemonicDecl>) |
            (dsl::peek(Common::Keyword<"FLAGS">::rule) >> dsl::p<FlagsDecl>) |
            (dsl::peek(Common::Keyword<"IMPLICIT_DEFS">::rule) >> dsl::p<ImplicitDefsDecl>) |
            (dsl::peek(Common::Keyword<"IMPLICIT_USES">::rule) >> dsl::p<ImplicitUsesDecl>) |
            (dsl::peek(Common::Keyword<"ENCODING">::rule) >> dsl::p<EncodingDeclItem>);

    static constexpr auto value = lexy::forward<ItemVariant>;
};

/**
 * Parses one `target_inst name(operands) { body }` declaration and folds the body into a
 * TargetInstDecl.
 */
struct TargetInstDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses a `,`-separated list of target operands into a PMR vector.
     */
    struct NonEmptyOperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::list(dsl::p<TargetOperand>, dsl::sep(dsl::lit_c<','>));
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>>;
    };

    /**
     * Parses the parenthesized operand signature, allowing an empty `()` list.
     */
    struct OperandList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::parenthesized(
                dsl::opt(dsl::peek(dsl::ascii::alpha_digit_underscore) >> dsl::p<NonEmptyOperandList>));
        static constexpr auto value = lexy::callback<std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>>(
                [](std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl> list) { return list; },
                [](lexy::nullopt) { return std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl>{}; });
    };

    /**
     * Parses the curly-braced body as a list of BodyItem variants.
     */
    struct BodyList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<BodyItem>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<BodyItem::ItemVariant>>;
    };

    static constexpr auto rule = Common::Keyword<"target_inst">::rule >>
            (dsl::p<Common::Identifier> + dsl::p<OperandList> + dsl::p<BodyList> + dsl::opt(dsl::lit_c<';'>));

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetInstDecl>(
            [](Ast::Common::Identifier name,
               std::pmr::vector<Ast::TargetInstDef::TargetOperandDecl> operands,
               std::pmr::vector<BodyItem::ItemVariant> bodyItems,
               auto...)
            {
                Ast::TargetInstDef::TargetInstDecl decl;
                decl.m_instName = std::move(name);
                decl.m_operands = std::move(operands);

                for (auto &item : bodyItems)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, BodyItem::TagMnemonic>)
                                    decl.m_mnemonic = std::move(val.val);
                                else if constexpr (std::is_same_v<T, BodyItem::TagFlags>)
                                    decl.m_flags = std::move(val.val);
                                else if constexpr (std::is_same_v<T, BodyItem::TagImplicitDefs>)
                                    decl.m_implicitDefs = std::move(val.val);
                                else if constexpr (std::is_same_v<T, BodyItem::TagImplicitUses>)
                                    decl.m_implicitUses = std::move(val.val);
                                else if constexpr (std::is_same_v<T, BodyItem::TagEncoding>)
                                    decl.m_encoding = std::move(val.val);
                            },
                            item);
                }
                return decl;
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
 * Recognizes a `target_inst` declaration and forwards the parsed TargetInstDecl.
 */
struct InstItemParser
{
    static constexpr auto rule = dsl::peek(Common::Keyword<"target_inst">::rule) >> dsl::p<TargetInstDecl>;
    static constexpr auto value = lexy::forward<Ast::TargetInstDef::TargetInstDecl>;
};

/**
 * Parses a whole `.idf` file as an optional target header plus an EOF-terminated instruction list.
 */
struct TargetInstFile
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the sequence of target instruction declarations into a PMR vector.
     */
    struct InstList
    {
        static constexpr auto rule = dsl::list(dsl::p<InstItemParser>);
        static constexpr auto value = Common::PmrAsList<std::pmr::vector<Ast::TargetInstDef::TargetInstDecl>>;
    };

    static constexpr auto rule = dsl::terminator(dsl::eof)(dsl::opt(dsl::p<TargetHeader>) + dsl::p<InstList>);

    static constexpr auto value = lexy::callback<Ast::TargetInstDef::TargetInstFile>(
            [](auto targetOpt, std::pmr::vector<Ast::TargetInstDef::TargetInstDecl> insts)
            {
                Ast::TargetInstDef::TargetInstFile file;
                if constexpr (std::is_same_v<std::decay_t<decltype(targetOpt)>, Ast::Common::Identifier>)
                {
                    file.m_targetName = std::move(targetOpt);
                }
                file.m_instructions = std::move(insts);
                return file;
            });
};

} // namespace DSL::Parser::TargetInstDef

#endif // EZDSL_PARSER_TARGET_INST_DEF_LANG_H
