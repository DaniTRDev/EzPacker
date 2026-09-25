#ifndef EZDSLSEMA_REGISTER_SYMBOLS_H
#define EZDSLSEMA_REGISTER_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace DSL::Ast::RegisterDef
{
struct RegisterBankDecl;
struct RegisterClassDecl;
struct RegisterDecl;
struct SpecialRegDecl;
} // namespace DSL::Ast::RegisterDef

namespace Symbols
{

/**
 * Semantic symbol for a `register_bank` declaration (.tdesc).
 * The flattened name/target fields are retained for consumers and tests; code generation resolves
 * banks through the TargetDescSymbol's AST node.
 */
struct RegisterBankSymbol
{
    std::string_view m_name;                                             // Bank name.
    std::string_view m_target;                                           // Target the bank belongs to.
    const DSL::Ast::RegisterDef::RegisterBankDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

/**
 * Semantic symbol for a register class declared inside a bank (.tdesc).
 * The first three fields mirror the AST for consumers/tests; code generation reads m_astNode.
 */
struct RegisterClassSymbol
{
    std::string_view m_name;                                              // Class name.
    std::string_view m_bankName;                                          // Owning bank name.
    uint32_t m_bitSize{ 0 };                                              // Class width in bits.
    const DSL::Ast::RegisterDef::RegisterClassDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

/**
 * Semantic symbol for a physical register declaration (.tdesc), including its explicit
 * hardware encoding. The scalar fields mirror the AST for consumers/tests; the register-info
 * generator resolves registers through the backing AST node.
 */
struct RegisterSymbol
{
    std::string_view m_name;                                         // Canonical (widest) assembly name.
    std::string_view m_bankName;                                     // Owning bank name.
    uint32_t m_hwEncoding{ 0 };                                      // Hardware encoding within the bank.
    const DSL::Ast::RegisterDef::RegisterDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

/**
 * Semantic symbol for a non-allocatable pseudo register declared in `special { ... }`.
 * The name/target/id fields mirror the AST for consumers/tests; code generation reads m_astNode.
 */
struct SpecialRegisterSymbol
{
    std::string_view m_name;                                           // Pseudo-register name.
    std::string_view m_target;                                         // Target it belongs to.
    uint32_t m_id{ 0 };                                                // Reserved pseudo-register id.
    const DSL::Ast::RegisterDef::SpecialRegDecl *m_astNode{ nullptr }; // Backing AST declaration.
};

} // namespace Symbols

#endif // EZDSLSEMA_REGISTER_SYMBOLS_H
