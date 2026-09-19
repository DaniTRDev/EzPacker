#ifndef EZDSLLEXER_REGISTER_DEF_LANG_AST_H
#define EZDSLLEXER_REGISTER_DEF_LANG_AST_H

#include "CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::RegisterDef
{

/**
 * Declares a register class within a register bank (e.g. GPR8: 8).
 */
struct RegisterClassDecl
{
    Common::Identifier m_name;
    Common::IntegerLiteral m_bitSize;
};

/**
 * Declares that every register of the wide class has a same-encoding slice in the narrow class.
 * Represented textually as: WIDE <: NARROW.
 */
struct SubRegisterEdge
{
    Common::Identifier m_wideClass;
    Common::Identifier m_narrowClass;
};

/**
 * Maps a declared register class to the printable assembly name used at that width.
 */
struct RegisterNameBinding
{
    Common::Identifier m_asmName;
    Common::Identifier m_className;
};

/**
 * Declares one physical register: canonical name, hardware encoding, and the per-class
 * assembly-name aliases (sub-registers).
 */
struct RegisterDecl
{
    Common::Identifier m_canonicalName;
    Common::IntegerLiteral m_encoding;
    std::pmr::vector<RegisterNameBinding> m_names;
};

/**
 * Declares a register bank together with its classes, sub-register relations, and
 * physical registers.
 */
struct RegisterBankDecl
{
    Common::Identifier m_name;
    std::pmr::vector<RegisterClassDecl> m_classes;
    std::pmr::vector<SubRegisterEdge> m_subRegisterEdges;
    std::pmr::vector<RegisterDecl> m_registers;
};

/**
 * Declares a non-allocatable pseudo register referenced by the ABI (e.g. rip: 16).
 */
struct SpecialRegDecl
{
    Common::Identifier m_name;
    Common::IntegerLiteral m_id;
};

/**
 * Root of a parsed .reg file.
 */
struct RegisterFile
{
    Common::Identifier m_target;
    std::pmr::vector<RegisterBankDecl> m_banks;
    std::pmr::vector<SpecialRegDecl> m_specialRegs;
};

} // namespace DSL::Ast::RegisterDef

#endif // EZDSLLEXER_REGISTER_DEF_LANG_AST_H
