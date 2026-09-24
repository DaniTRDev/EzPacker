#ifndef EZDSLLEXER_ENCODING_DEF_LANG_AST_H
#define EZDSLLEXER_ENCODING_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"
#include <optional>

namespace DSL::Ast::Encoding
{

/**
 * Binds a declared instruction operand name to a target-defined encoding field.
 *
 * The field name is owned by the target's encoding dialect (e.g. `reg`, `rm_reg`,
 * `rd`, `shamt`); the shared AST never assigns meaning to it.
 */
struct OperandBinding
{
    Common::Identifier m_operand; // Declared instruction operand name.
    Common::Identifier m_field;   // Target-defined field/slot name.
};

/**
 * Arch-neutral value of an ENCODING directive.
 *
 * Each architecture owns the interpretation of the keys and values; the shared
 * AST only understands their shapes.
 */
using Value = std::variant<std::monostate,
                           bool,
                           int64_t,
                           Common::Identifier,
                           std::pmr::vector<uint8_t>,        // [0x0F, 0x58]
                           std::pmr::vector<OperandBinding>>; // operands { op => field; ... }

/**
 * One `key : value` directive inside a generic ENCODING block.
 */
struct Directive
{
    Common::Identifier m_key; // e.g. "form", "opcode", "rex_w", "rd", "shamt".
    Value m_value;
};

/**
 * Declarative description of an instruction's machine encoding.
 *
 * The container is arch-neutral: an optional backend selector plus target-defined
 * directives. Adding an ISA never grows this shared node.
 */
struct EncodingDecl
{
    std::optional<Common::Identifier> m_backend; // Explicit dialect selector, e.g. "x86_64".
    std::pmr::vector<Directive> m_directives;    // Target-defined encoding directives.
};

} // namespace DSL::Ast::Encoding

#endif // EZDSLLEXER_ENCODING_DEF_LANG_AST_H
