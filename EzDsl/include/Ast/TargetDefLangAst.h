#ifndef EZDSL_TARGET_DEF_LANG_AST_H
#define EZDSL_TARGET_DEF_LANG_AST_H

#include "Ast/CommonAstNodes.h"
#include "EzDslCommon.h"

#include <vector>

namespace DSL::Ast::TargetDef
{

/**
 * Register declaration with optional parent register alias and bit offsets:
 *   - TargetRegister(rax,    ,    64, 0)
 *   - TargetRegister(eax, rax,    32, 0)
 */
struct TargetRegister
{
    Common::Identifier m_name;
    Common::Identifier m_parentName;
    Common::IntegerLiteral m_size;
    Common::IntegerLiteral m_offset;
};

/**
 * TargetRegisterClass(GPR64, { TargetRegister(rax, , 64, 0), ... })
 */
struct TargetRegisterClass
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegister> m_registers;
};

/**
 * TargetRegisterBank(GPR, { TargetRegisterClass(GPR64), ... })
 */
struct TargetRegisterBank
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegisterClass> m_classes;
};

/**
 * Target include directive:
 *   - include idef "instructions.idf";
 */
struct TargetIncFile
{
    Common::Identifier m_inclusionType;
    Common::StringLiteral m_path;
};

/**
 * Target translation unit root:
 *   - target x86_64 { include idef "..."; bank GPR { ... }; };
 */
struct TargetDef
{
    Common::Identifier m_name;
    std::pmr::vector<TargetIncFile> m_inclusions;
    std::pmr::vector<TargetRegisterBank> m_regBanks;
};

} // namespace DSL::Ast::TargetDef

#endif // EZDSL_TARGET_DEF_LANG_AST_H