#ifndef EZDSLSEMA_REGISTER_SYMBOLS_H
#define EZDSLSEMA_REGISTER_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace DSL::Ast::RegisterDef
{
struct RegisterBankDecl;
struct RegisterClassDecl;
struct RegisterDecl;
struct RegisterFile;
struct SpecialRegDecl;
} // namespace DSL::Ast::RegisterDef

namespace Symbols
{

/**
 * Semantic symbol for a `register_bank` declaration (.reg).
 */
struct RegisterBankSymbol
{
    std::string_view m_name;
    std::string_view m_target;
    const DSL::Ast::RegisterDef::RegisterBankDecl *m_astNode{ nullptr };
};

/**
 * Semantic symbol for a register class declared inside a bank (.reg).
 */
struct RegisterClassSymbol
{
    std::string_view m_name;
    std::string_view m_bankName;
    uint32_t m_bitSize{ 0 };
    const DSL::Ast::RegisterDef::RegisterClassDecl *m_astNode{ nullptr };
};

/**
 * Semantic symbol for a physical register declaration (.reg), including its explicit
 * hardware encoding.
 */
struct RegisterSymbol
{
    std::string_view m_name;      // Canonical (widest) assembly name.
    std::string_view m_bankName;
    uint32_t m_hwEncoding{ 0 };
    const DSL::Ast::RegisterDef::RegisterDecl *m_astNode{ nullptr };
};

/**
 * Semantic symbol for a non-allocatable pseudo register declared in `special { ... }`.
 */
struct SpecialRegisterSymbol
{
    std::string_view m_name;
    std::string_view m_target;
    uint32_t m_id{ 0 };
    const DSL::Ast::RegisterDef::SpecialRegDecl *m_astNode{ nullptr };
};

/**
 * Root symbol holding the whole parsed register file, used by the register-info generator.
 */
struct RegisterFileSymbol
{
    std::string_view m_target;
    const DSL::Ast::RegisterDef::RegisterFile *m_astNode{ nullptr };
};

} // namespace Symbols

#endif // EZDSLSEMA_REGISTER_SYMBOLS_H
