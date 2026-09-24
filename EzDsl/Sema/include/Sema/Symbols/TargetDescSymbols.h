#ifndef EZDSLSEMA_TARGET_DESC_SYMBOLS_H
#define EZDSLSEMA_TARGET_DESC_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace DSL::Ast::TargetDesc
{
struct TargetDescDecl;
struct TargetDescFile;
} // namespace DSL::Ast::TargetDesc

namespace Symbols
{

/**
 * Semantic symbol for a parsed `.tdesc` target description manifest.
 */
struct TargetDescSymbol
{
    std::string_view m_name;                                          // Target name.
    const DSL::Ast::TargetDesc::TargetDescDecl *m_astNode{ nullptr }; // Backing AST manifest.
};

} // namespace Symbols

#endif // EZDSLSEMA_TARGET_DESC_SYMBOLS_H
