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
    Common::Identifier m_name;
    Common::StringLiteral m_symbol;
};

/**
 * Binds a well-known strategy slot (e.g. frame_lowerer) to a concrete C++ type name.
 */
struct ComponentBinding
{
    Common::Identifier m_slot;
    Common::Identifier m_type;
};

/**
 * Declarative description of a target, referencing sibling config files and target constants.
 */
struct TargetDescDecl
{
    Common::Identifier m_name;

    std::optional<Common::StringLiteral> m_registers;
    std::optional<Common::StringLiteral> m_instructions;
    std::pmr::vector<Common::StringLiteral> m_callingConvs;

    std::optional<Common::IntegerLiteral> m_pointerSize;
    std::optional<Common::IntegerLiteral> m_stackSlot;

    std::optional<Common::Identifier> mInstructionPointer;
    std::optional<Common::Identifier> mMemDispType;

    std::pmr::vector<Common::Identifier> mObjectFormats;
    std::optional<Common::Identifier> mDefaultCallingConv;

    std::pmr::vector<LibcallEntry> mLibcalls;
    std::pmr::vector<ComponentBinding> mComponents;
};

/**
 * Root of a parsed .tdesc file.
 */
struct TargetDescFile : TargetDescDecl
{
};

} // namespace DSL::Ast::TargetDesc

#endif // EZDSLLEXER_TARGET_DESC_DEF_LANG_AST_H
