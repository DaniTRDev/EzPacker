#ifndef EZDSLSYMBOLS_TYPE_SYMBOLS_H
#define EZDSLSYMBOLS_TYPE_SYMBOLS_H

#include "SymbolCommon.h"

/**
 * Forward declarations.
 */
namespace DSL::Ast::TypeDef
{
enum class TypeKind : uint8_t;
}; // namespace DSL::Ast::TypeDef

namespace Symbols
{
/**
 * Semantic symbol for a parsed primitive or special IR type declaration (.tyf).
 */
struct TypeSymbol
{
    std::string_view m_name;
    DSL::Ast::TypeDef::TypeKind m_kind;
    uint32_t m_bitWidth{ 0 };
    uint32_t m_alignment{ 0 };
    uint8_t m_compactId{ 0 }; // Fast O(1) index for table driven legalizer/selector.
};
}; // namespace Symbols

#endif // EZDSLSYMBOLS_TYPE_SYMBOLS_H