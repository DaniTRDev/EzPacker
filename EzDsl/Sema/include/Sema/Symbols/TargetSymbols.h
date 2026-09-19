#ifndef EZDSLSEMA_TARGET_SYMBOLS_H
#define EZDSLSEMA_TARGET_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"
#include "Ast/TargetInstDefLangAst.h"

namespace Symbols
{

struct TargetOperandSymbol
{
    std::string_view m_regClassOrType;
    std::string_view m_name;
    DSL::Ast::TargetInstDef::OperandDirection m_direction;
};

struct TargetInstructionSymbol
{
    std::string_view m_name;
    std::string_view m_mnemonic;
    std::pmr::vector<TargetOperandSymbol> m_operands;
    std::pmr::vector<std::string_view> m_flags;
    std::pmr::vector<std::string_view> m_implicitDefs;
    std::pmr::vector<std::string_view> m_implicitUses;
    std::optional<DSL::Ast::TargetInstDef::EncodingDecl> m_encoding;

    bool hasFlag(std::string_view flag) const noexcept
    {
        for (const auto &f : m_flags)
        {
            if (f == flag)
                return true;
        }
        return false;
    }
};

} // namespace Symbols

#endif // EZDSLSEMA_TARGET_SYMBOLS_H
