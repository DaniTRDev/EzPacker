#ifndef EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H
#define EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"
#include "Ast/InstructionSelectDefLangAst.h"

namespace Symbols
{

/**
 * Semantic symbol for a declared addressing mode (.isf).
 */
struct AddrModeSymbol
{
    std::string_view m_name;                                                  // Addressing mode name.
    const DSL::Ast::InstructionSelectDef::AddrModeDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

/**
 * Semantic symbol for a declared instruction selection pattern (.isf).
 */
struct SelectionPatternSymbol
{
    std::string_view m_name;                                                      // Pattern name.
    uint32_t m_cost{ 1 };                                                         // Relative selection cost.
    const DSL::Ast::InstructionSelectDef::SelectionPattern *m_astNode{ nullptr }; // Backing AST declaration.
};

} // namespace Symbols

#endif // EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H
