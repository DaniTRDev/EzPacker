#ifndef EZDSL_TARGET_DESC_DEF_LANG_H
#define EZDSL_TARGET_DESC_DEF_LANG_H

#include "Ast/CommonAstNodes.h"
#include "Ast/TargetDescDefLangAst.h"
#include "EzDslLexerCommon.h"
#include "Parser/CommonParsers.h"

namespace DSL::Parser::TargetDesc
{
namespace dsl = ::lexy::dsl;

using Ast::TargetDesc::ComponentBinding;
using Ast::TargetDesc::ExtensionDef;
using Ast::TargetDesc::LibcallEntry;
using Ast::TargetDesc::TargetDescFile;

/**
 * Body field variant produced by each `.tdesc` declaration.
 */
using TargetDescItem =
        std::variant<std::pair<Common::Keyword<"registers">, Ast::Common::StringLiteral>,
                     std::pair<Common::Keyword<"instructions">, Ast::Common::StringLiteral>,
                     std::pair<Common::Keyword<"calling_convs">, std::pmr::vector<Ast::Common::StringLiteral>>,
                     std::pair<Common::Keyword<"pointer_size">, Ast::Common::IntegerLiteral>,
                     std::pair<Common::Keyword<"stack_slot">, Ast::Common::IntegerLiteral>,
                     std::pair<Common::Keyword<"instruction_pointer">, Ast::Common::Identifier>,
                     std::pair<Common::Keyword<"mem_disp_type">, Ast::Common::Identifier>,
                     std::pair<Common::Keyword<"object_formats">, std::pmr::vector<Ast::Common::Identifier>>,
                     std::pair<Common::Keyword<"default_calling_conv">, Ast::Common::Identifier>,
                     std::pair<Common::Keyword<"libcalls">, std::pmr::vector<LibcallEntry>>,
                     std::pair<Common::Keyword<"components">, std::pmr::vector<ComponentBinding>>,
                     std::pair<Common::Keyword<"extensions">, std::pmr::vector<ExtensionDef>>>;

/**
 * Parses a `[ "a", "b" ]` list of string literals into a PMR vector.
 */
struct StringList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::square_bracketed.list(dsl::p<Common::StringLiteral>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<Ast::Common::StringLiteral>;
};

/**
 * Parses a `[ a, b ]` list of identifiers into a PMR vector.
 */
struct IdentifierList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::square_bracketed.list(dsl::p<Common::Identifier>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = Common::PmrAsList<Ast::Common::Identifier>;
};

/**
 * Parses `registers: "path"` into the registers field item.
 */
struct RegistersDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"registers">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::StringLiteral>);
    static constexpr auto value =
            lexy::callback<TargetDescItem>([](Ast::Common::StringLiteral path)
                                           { return std::make_pair(Common::Keyword<"registers">{}, std::move(path)); });
};

/**
 * Parses `instructions: "path"` into the instructions field item.
 */
struct InstructionsDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"instructions">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::StringLiteral>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](Ast::Common::StringLiteral path)
            { return std::make_pair(Common::Keyword<"instructions">{}, std::move(path)); });
};

/**
 * Parses `calling_convs: [ "path", ... ]` into the calling-convention paths item.
 */
struct CallingConvsDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"calling_convs">::rule >> (dsl::lit_c<':'> >> dsl::p<StringList>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](std::pmr::vector<Ast::Common::StringLiteral> paths)
            { return std::make_pair(Common::Keyword<"calling_convs">{}, std::move(paths)); });
};

/**
 * Parses `pointer_size: N` into the pointer-size field item.
 */
struct PointerSizeDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"pointer_size">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
    static constexpr auto value =
            lexy::callback<TargetDescItem>([](Ast::Common::IntegerLiteral v)
                                           { return std::make_pair(Common::Keyword<"pointer_size">{}, std::move(v)); });
};

/**
 * Parses `stack_slot: N` into the stack-slot-size field item.
 */
struct StackSlotDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"stack_slot">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::IntegerLiteral>);
    static constexpr auto value =
            lexy::callback<TargetDescItem>([](Ast::Common::IntegerLiteral v)
                                           { return std::make_pair(Common::Keyword<"stack_slot">{}, std::move(v)); });
};

/**
 * Parses `instruction_pointer: REG` into the instruction-pointer field item.
 */
struct InstructionPointerDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"instruction_pointer">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](Ast::Common::Identifier id)
            { return std::make_pair(Common::Keyword<"instruction_pointer">{}, std::move(id)); });
};

/**
 * Parses `mem_disp_type: TYPE` into the memory-displacement type field item.
 */
struct MemDispTypeDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"mem_disp_type">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](Ast::Common::Identifier id)
            { return std::make_pair(Common::Keyword<"mem_disp_type">{}, std::move(id)); });
};

/**
 * Parses `object_formats: [ fmt, ... ]` into the object-format list field item.
 */
struct ObjectFormatsDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"object_formats">::rule >> (dsl::lit_c<':'> >> dsl::p<IdentifierList>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](std::pmr::vector<Ast::Common::Identifier> formats)
            { return std::make_pair(Common::Keyword<"object_formats">{}, std::move(formats)); });
};

/**
 * Parses `default_calling_conv: NAME` into the default-convention field item.
 */
struct DefaultCallingConvDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"default_calling_conv">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::Identifier>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](Ast::Common::Identifier id)
            { return std::make_pair(Common::Keyword<"default_calling_conv">{}, std::move(id)); });
};

/**
 * Parses one `id: "symbol"` libcall mapping.
 */
struct LibcallEntryParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::StringLiteral>;
    static constexpr auto value = lexy::callback<LibcallEntry>(
            [](Ast::Common::Identifier name, Ast::Common::StringLiteral symbol)
            { return LibcallEntry{ .m_name = std::move(name), .m_symbol = std::move(symbol) }; });
};

/**
 * Parses `libcalls { id: "symbol"; ... }` into the libcall list field item.
 */
struct LibcallsDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the optional `;`-separated libcall entries into a PMR vector.
     */
    struct LibcallList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                dsl::curly_bracketed.opt_list(dsl::p<LibcallEntryParser>, dsl::sep(dsl::lit_c<';'>));
        static constexpr auto value = Common::PmrAsList<LibcallEntry>;
    };

    static constexpr auto rule = Common::Keyword<"libcalls">::rule >> dsl::p<LibcallList>;
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](std::pmr::vector<LibcallEntry> entries)
            { return std::make_pair(Common::Keyword<"libcalls">{}, std::move(entries)); });
};

/**
 * Parses one `slot: Type` component binding.
 */
struct ComponentEntryParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::lit_c<':'> + dsl::p<Common::Identifier>;
    static constexpr auto value = lexy::callback<ComponentBinding>(
            [](Ast::Common::Identifier slot, Ast::Common::Identifier type)
            { return ComponentBinding{ .m_slot = std::move(slot), .m_type = std::move(type) }; });
};

/**
 * Parses `components { slot: Type; ... }` into the component-binding list field item.
 */
struct ComponentsDecl
{
    static constexpr auto whitespace = Common::Whitespace;

    /**
     * Parses the optional `;`-separated component bindings into a PMR vector.
     */
    struct ComponentList
    {
        static constexpr auto whitespace = Common::Whitespace;
        static constexpr auto rule =
                dsl::curly_bracketed.opt_list(dsl::p<ComponentEntryParser>, dsl::sep(dsl::lit_c<';'>));
        static constexpr auto value = Common::PmrAsList<ComponentBinding>;
    };

    static constexpr auto rule = Common::Keyword<"components">::rule >> dsl::p<ComponentList>;
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](std::pmr::vector<ComponentBinding> bindings)
            { return std::make_pair(Common::Keyword<"components">{}, std::move(bindings)); });
};

/**
 * Item within an extension block: default, implies, or description.
 */
using ExtensionFieldItem =
        std::variant<std::pair<Common::Keyword<"default">, Ast::Common::BooleanLiteral>,
                     std::pair<Common::Keyword<"implies">, std::pmr::vector<Ast::Common::Identifier>>,
                     std::pair<Common::Keyword<"description">, Ast::Common::StringLiteral>>;

struct ExtensionDefaultDecl
{
    static constexpr auto rule = Common::Keyword<"default">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::BooleanLiteral>);
    static constexpr auto value = lexy::callback<ExtensionFieldItem>(
            [](Ast::Common::BooleanLiteral b)
            { return std::make_pair(Common::Keyword<"default">{}, std::move(b)); });
};

struct ExtensionImpliesDecl
{
    static constexpr auto rule = Common::Keyword<"implies">::rule >>
            (dsl::lit_c<':'> >> dsl::p<IdentifierList>);
    static constexpr auto value = lexy::callback<ExtensionFieldItem>(
            [](std::pmr::vector<Ast::Common::Identifier> ids)
            { return std::make_pair(Common::Keyword<"implies">{}, std::move(ids)); });
};

struct ExtensionDescriptionDecl
{
    static constexpr auto rule = Common::Keyword<"description">::rule >>
            (dsl::lit_c<':'> >> dsl::p<Common::StringLiteral>);
    static constexpr auto value = lexy::callback<ExtensionFieldItem>(
            [](Ast::Common::StringLiteral s)
            { return std::make_pair(Common::Keyword<"description">{}, std::move(s)); });
};

struct ExtensionFieldEntry
{
    static constexpr auto rule = []
    {
        auto def = dsl::peek(Common::Keyword<"default">::rule) >> dsl::p<ExtensionDefaultDecl>;
        auto imp = dsl::peek(Common::Keyword<"implies">::rule) >> dsl::p<ExtensionImpliesDecl>;
        auto desc = dsl::peek(Common::Keyword<"description">::rule) >> dsl::p<ExtensionDescriptionDecl>;
        auto inner = def | imp | desc;
        return inner + dsl::opt(dsl::lit_c<';'>);
    }();
    static constexpr auto value = lexy::callback<ExtensionFieldItem>([](ExtensionFieldItem item, auto...) { return item; });
};

struct ExtensionFieldList
{
    static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<ExtensionFieldEntry>);
    static constexpr auto value = Common::PmrAsList<ExtensionFieldItem>;
};

struct ExtensionBlockEntryParser
{
    static constexpr auto rule = dsl::p<Common::Identifier> + dsl::opt(dsl::p<ExtensionFieldList>) + dsl::opt(dsl::lit_c<';'>);
    static constexpr auto value = lexy::callback<ExtensionDef>(
            [](Ast::Common::Identifier name, std::pmr::vector<ExtensionFieldItem> fields, auto...)
            {
                ExtensionDef def{};
                def.m_name = std::move(name);
                for (auto &f : fields)
                {
                    std::visit(
                            [&](auto &&val)
                            {
                                using T = std::decay_t<decltype(val.first)>;
                                if constexpr (std::is_same_v<T, Common::Keyword<"default">>)
                                    def.m_default = std::move(val.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"implies">>)
                                    def.m_implies = std::move(val.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"description">>)
                                    def.m_description = std::move(val.second);
                            },
                            f);
                }
                return def;
            },
            [](Ast::Common::Identifier name, lexy::nullopt, auto...)
            {
                ExtensionDef def{};
                def.m_name = std::move(name);
                return def;
            });
};

struct ExtensionBlockList
{
    static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<ExtensionBlockEntryParser>);
    static constexpr auto value = Common::PmrAsList<ExtensionDef>;
};

struct ExtensionsDecl
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = Common::Keyword<"extensions">::rule >>
            ((dsl::lit_c<':'> >> dsl::p<IdentifierList>) | dsl::p<ExtensionBlockList>);
    static constexpr auto value = lexy::callback<TargetDescItem>(
            [](std::pmr::vector<Ast::Common::Identifier> ids)
            {
                std::pmr::vector<ExtensionDef> defs;
                defs.reserve(ids.size());
                for (auto &id : ids)
                {
                    ExtensionDef def{};
                    def.m_name = std::move(id);
                    defs.push_back(std::move(def));
                }
                return std::make_pair(Common::Keyword<"extensions">{}, std::move(defs));
            },
            [](std::pmr::vector<ExtensionDef> defs)
            { return std::make_pair(Common::Keyword<"extensions">{}, std::move(defs)); });
};

/**
 * Dispatches one body field of a target description to the matching `*Decl` sub-parser.
 */
struct BodyEntry
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = []
    {
        auto registers = dsl::peek(Common::Keyword<"registers">::rule) >> dsl::p<RegistersDecl>;
        auto instructions = dsl::peek(Common::Keyword<"instructions">::rule) >> dsl::p<InstructionsDecl>;
        auto convs = dsl::peek(Common::Keyword<"calling_convs">::rule) >> dsl::p<CallingConvsDecl>;
        auto ptr = dsl::peek(Common::Keyword<"pointer_size">::rule) >> dsl::p<PointerSizeDecl>;
        auto stack = dsl::peek(Common::Keyword<"stack_slot">::rule) >> dsl::p<StackSlotDecl>;
        auto ip = dsl::peek(Common::Keyword<"instruction_pointer">::rule) >> dsl::p<InstructionPointerDecl>;
        auto disp = dsl::peek(Common::Keyword<"mem_disp_type">::rule) >> dsl::p<MemDispTypeDecl>;
        auto formats = dsl::peek(Common::Keyword<"object_formats">::rule) >> dsl::p<ObjectFormatsDecl>;
        auto dcc = dsl::peek(Common::Keyword<"default_calling_conv">::rule) >> dsl::p<DefaultCallingConvDecl>;
        auto libcalls = dsl::peek(Common::Keyword<"libcalls">::rule) >> dsl::p<LibcallsDecl>;
        auto components = dsl::peek(Common::Keyword<"components">::rule) >> dsl::p<ComponentsDecl>;
        auto extensions = dsl::peek(Common::Keyword<"extensions">::rule) >> dsl::p<ExtensionsDecl>;

        auto inner = registers | instructions | convs | ptr | stack | ip | disp | formats | dcc | libcalls | components | extensions;
        return inner + dsl::opt(dsl::lit_c<';'>);
    }();
    static constexpr auto value = lexy::callback<TargetDescItem>([](TargetDescItem item, auto...) { return item; });
};

/**
 * Parses the curly-braced list of body fields into PMR TargetDescItem variants.
 */
struct BodyList
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule = dsl::curly_bracketed.list(dsl::p<BodyEntry>);
    static constexpr auto value = Common::PmrAsList<TargetDescItem>;
};

/**
 * Root rule for a `.tdesc` file.
 */
struct TargetDescFileParser
{
    static constexpr auto whitespace = Common::Whitespace;
    static constexpr auto rule =
            dsl::terminator(dsl::eof)(Common::Keyword<"target">::rule >> (dsl::p<Common::Identifier> + dsl::p<BodyList>));
    static constexpr auto value = lexy::callback<TargetDescFile>(
            [](Ast::Common::Identifier name, std::pmr::vector<TargetDescItem> items)
            {
                TargetDescFile file{};
                file.m_name = std::move(name);

                for (auto &item : items)
                {
                    std::visit(
                            [&](auto &&field)
                            {
                                using T = std::decay_t<decltype(field.first)>;
                                if constexpr (std::is_same_v<T, Common::Keyword<"registers">>)
                                    file.m_registers = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"instructions">>)
                                    file.m_instructions = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"calling_convs">>)
                                    file.m_callingConvs = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"pointer_size">>)
                                    file.m_pointerSize = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"stack_slot">>)
                                    file.m_stackSlot = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"instruction_pointer">>)
                                    file.mInstructionPointer = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"mem_disp_type">>)
                                    file.mMemDispType = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"object_formats">>)
                                    file.mObjectFormats = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"default_calling_conv">>)
                                    file.mDefaultCallingConv = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"libcalls">>)
                                    file.mLibcalls = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"components">>)
                                    file.mComponents = std::move(field.second);
                                else if constexpr (std::is_same_v<T, Common::Keyword<"extensions">>)
                                    file.m_extensions = std::move(field.second);
                            },
                            item);
                }
                return file;
            });
};

} // namespace DSL::Parser::TargetDesc

#endif // EZDSL_TARGET_DESC_DEF_LANG_H
