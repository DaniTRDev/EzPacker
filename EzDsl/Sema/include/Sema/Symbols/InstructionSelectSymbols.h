#ifndef EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H
#define EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"
#include "Ast/InstructionSelectDefLangAst.h"

namespace Symbols
{

struct AddrModeSymbol
{
    std::string_view m_name;
    const DSL::Ast::InstructionSelectDef::AddrModeDecl *m_astNode{ nullptr };
};

struct SelectionPatternSymbol
{
    std::string_view m_name;
    uint32_t m_cost{ 1 };
    const DSL::Ast::InstructionSelectDef::SelectionPattern *m_astNode{ nullptr };
};

} // namespace Symbols

#endif // EZDSLSEMA_INSTRUCTION_SELECT_SYMBOLS_H
