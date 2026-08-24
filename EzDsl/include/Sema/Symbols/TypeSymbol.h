#ifndef EZDSL_TYPE_SYMBOL_H
#define EZDSL_TYPE_SYMBOL_H

#include "EzDslCommon.h"
#include "Ast/TypeDefLangAst.h"

namespace Sema::Symbols
{
/**
 * Data contained inside a type symbol.
 */
struct TypeSymbol
{
    DSL::Ast::TypeDef::TypeKind m_kind;
    uint32_t m_bitWidth;
    std::string_view m_name;
};
}; // namespace Sema::Symbols

#endif // EZDSL_TYPE_SYMBOL_H