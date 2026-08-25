#ifndef EZDSL_TARGET_DEF_LANG_AST_H
#define EZDSL_TARGET_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"

#include <vector>

namespace DSL::Ast::TargetDef
{

/**
 * Target architecture register definition with parent alias, bit size, and bit offset.
 *
 * Syntax:
 *   TargetRegister := RegName '(' ParentName? ',' BitSize ',' BitOffset ')'
 *   RegName        := Identifier
 *   ParentName     := Identifier
 *   BitSize        := IntegerLiteral
 *   BitOffset      := IntegerLiteral
 *
 * Examples:
 *   rax(, 64, 0)
 *   eax(rax, 32, 0)
 */
struct TargetRegister
{
    Common::Identifier m_name;
    Common::Identifier m_parentName;
    Common::IntegerLiteral m_size;
    Common::IntegerLiteral m_offset;
};

/**
 * Register class group containing a list of target registers.
 *
 * Syntax:
 *   TargetRegisterClass := 'CLASS' '(' ClassName ( ',' TargetRegister ( ',' TargetRegister )* )? ')' ';'
 *   ClassName           := Identifier
 *
 * Examples:
 *   CLASS(GPR64, rax(, 64, 0), rbx(, 64, 0));
 *   CLASS(EMPTY);
 */
struct TargetRegisterClass
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegister> m_registers;
};

/**
 * Register bank AST node grouping multiple register classes.
 *
 * Syntax:
 *   TargetRegisterBank := 'bank' BankName '{' ( TargetRegisterClass )* '}' ';'?
 *   BankName           := Identifier
 *
 * Example:
 *   bank GPR {
 *       CLASS(GPR64, rax(, 64, 0), rcx(, 64, 0));
 *       CLASS(GPR32, eax(rax, 32, 0), ecx(rcx, 32, 0));
 *   };
 */
struct TargetRegisterBank
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegisterClass> m_classes;
};

/**
 * File type indicator for sub-language files included into a target definition.
 *
 * Valid include type keywords:
 *   'idf' -> InstructionDef
 *   'lad' -> LegalizeActionDef
 *   'lrd' -> LegalizeRuleDef
 *   'isf' -> InstructionSelDef
 */
enum class TargetIncludeFileType
{
    InstructionDef,
    LegalizeActionDef,
    LegalizeRuleDef,
    InstructionSelDef
};

/**
 * Target sub-file inclusion directive.
 *
 * Syntax:
 *   TargetIncFile := 'include' TargetIncFileType StringLiteral ';'
 *   TargetIncFileType := 'idf' | 'lad' | 'lrd' | 'isf'
 *
 * Examples:
 *   include idf "x86_64_instructions.idf";
 *   include lad "x86_64_legalize.lad";
 */
struct TargetIncFile
{
    TargetIncludeFileType m_inclusionType;
    Common::StringLiteral m_path;
};

/**
 * Top-level target definition root in a .tdf file.
 *
 * Syntax:
 *   TargetDef := 'target' TargetName '{' ( TargetIncFile | TargetRegisterBank )* '}' ';'? EOF
 *   TargetName := Identifier
 *
 * Example:
 *   target x86_64 {
 *       include idf "x86_instructions.idf";
 *       bank GPR {
 *           CLASS(GPR64, rax(, 64, 0));
 *       };
 *   };
 */
struct TargetDef
{
    Common::Identifier m_name;
    std::pmr::vector<TargetIncFile> m_inclusions;
    std::pmr::vector<TargetRegisterBank> m_regBanks;
};

} // namespace DSL::Ast::TargetDef

#endif // EZDSL_TARGET_DEF_LANG_AST_H