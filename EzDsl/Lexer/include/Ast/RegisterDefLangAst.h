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
    Common::Identifier m_name;        // Class name (e.g. GPR8).
    Common::IntegerLiteral m_bitSize; // Width in bits shared by all registers in the class.
};

/**
 * Declares that every register of the wide class has a same-encoding slice in the narrow class.
 * Represented textually as: WIDE <: NARROW.
 */
struct SubRegisterEdge
{
    Common::Identifier m_wideClass;   // Wider (parent) class.
    Common::Identifier m_narrowClass; // Narrower (sub-register) class carved from the wide class.
};

/**
 * Maps a declared register class to the printable assembly name used at that width.
 */
struct RegisterNameBinding
{
    Common::Identifier m_asmName;   // Printable assembly name at the bound width.
    Common::Identifier m_className; // Class whose width this name applies to.
};

/**
 * Declares one physical register: canonical name, hardware encoding, and the per-class
 * assembly-name aliases (sub-registers).
 */
struct RegisterDecl
{
    Common::Identifier m_canonicalName;            // Widest/primary assembly name.
    Common::IntegerLiteral m_encoding;             // Hardware encoding shared across the bank.
    std::pmr::vector<RegisterNameBinding> m_names; // Per-class assembly-name aliases.
};

/**
 * Declares a register bank together with its classes, sub-register relations, and
 * physical registers.
 */
struct RegisterBankDecl
{
    Common::Identifier m_name;                            // Bank name.
    std::pmr::vector<RegisterClassDecl> m_classes;        // Width classes declared in the bank.
    std::pmr::vector<SubRegisterEdge> m_subRegisterEdges; // Wide-to-narrow class relations.
    std::pmr::vector<RegisterDecl> m_registers;           // Physical registers in the bank.
};

/**
 * Declares a non-allocatable pseudo register referenced by the ABI (e.g. rip: 16).
 */
struct SpecialRegDecl
{
    Common::Identifier m_name;   // Pseudo-register name.
    Common::IntegerLiteral m_id; // Identifier reserved outside allocatable encodings.
};

/**
 * Root of a parsed .reg file.
 */
struct RegisterFile
{
    Common::Identifier m_target;                    // Target name this register file describes.
    std::pmr::vector<RegisterBankDecl> m_banks;     // All declared register banks.
    std::pmr::vector<SpecialRegDecl> m_specialRegs; // All declared special/pseudo registers.
};

} // namespace DSL::Ast::RegisterDef

#endif // EZDSLLEXER_REGISTER_DEF_LANG_AST_H
