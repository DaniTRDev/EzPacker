#ifndef EZDSLSEMA_CALLING_CONV_SYMBOLS_H
#define EZDSLSEMA_CALLING_CONV_SYMBOLS_H

#include "Ast/CallingConvDefLangAst.h"
#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace Symbols
{

struct CallingConvSymbol
{
    std::string_view m_name;
    const DSL::Ast::CallingConvDef::CallingConventionDecl *m_astNode{ nullptr };
};

} // namespace Symbols

#endif // EZDSLSEMA_CALLING_CONV_SYMBOLS_H
