#ifndef EZDSLLEXER_TARGET_DESC_DEF_LANG_AST_H
#define EZDSLLEXER_TARGET_DESC_DEF_LANG_AST_H

#include "CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::TargetDesc
{

/**
 * One libcall entry: a symbolic id mapped to its runtime symbol name.
 */
struct LibcallEntry
{
    Common::Identifier m_name;      // Symbolic libcall id referenced by legalization.
    Common::StringLiteral m_symbol; // Runtime/linker symbol the id maps to.
};

/**
 * Binds a well-known strategy slot (e.g. frame_lowerer) to a concrete C++ type name.
 */
struct ComponentBinding
{
    Common::Identifier m_slot; // Well-known strategy slot name (e.g. frame_lowerer).
    Common::Identifier m_type; // Concrete C++ type plugged into that slot.
};

/**
 * Declarative description of a target, referencing sibling config files and target constants.
 */
struct TargetDescDecl
{
    Common::Identifier m_name; // Target name (also its symbol).

    std::optional<Common::StringLiteral> m_registers;       // Path to the sibling `.reg` file.
    std::optional<Common::StringLiteral> m_instructions;    // Path to the sibling `.idf` file.
    std::pmr::vector<Common::StringLiteral> m_callingConvs; // Paths to calling convention files.

    std::optional<Common::IntegerLiteral> m_pointerSize; // Target pointer width in bits.
    std::optional<Common::IntegerLiteral> m_stackSlot;   // Natural stack slot size (power of two).

    std::optional<Common::Identifier> mInstructionPointer; // Special register naming the instruction pointer.
    std::optional<Common::Identifier> mMemDispType;        // Type used for memory displacement immediates.

    std::pmr::vector<Common::Identifier> mObjectFormats;   // Supported object file formats.
    std::optional<Common::Identifier> mDefaultCallingConv; // Convention used when none is specified.

    std::pmr::vector<LibcallEntry> mLibcalls;       // Libcall id-to-symbol mappings.
    std::pmr::vector<ComponentBinding> mComponents; // Strategy-slot to C++ type bindings.
};

/**
 * Root of a parsed .tdesc file.
 */
struct TargetDescFile : TargetDescDecl
{
};

} // namespace DSL::Ast::TargetDesc

#endif // EZDSLLEXER_TARGET_DESC_DEF_LANG_AST_H
