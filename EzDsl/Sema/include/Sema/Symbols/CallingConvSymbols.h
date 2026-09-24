#ifndef EZDSLSEMA_CALLING_CONV_SYMBOLS_H
#define EZDSLSEMA_CALLING_CONV_SYMBOLS_H

#include "Ast/CallingConvDefLangAst.h"
#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace Symbols
{

/**
 * Semantic symbol for a validated calling convention declaration (.ezcc / .ccd).
 */
struct CallingConvSymbol
{
    std::string_view m_name;                                                     // Convention name.
    const DSL::Ast::CallingConvDef::CallingConventionDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

} // namespace Symbols

#endif // EZDSLSEMA_CALLING_CONV_SYMBOLS_H
