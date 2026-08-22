#ifndef EZDSL_TARGET_DEF_LANG_AST_H
#define EZDSL_TARGET_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"

/**
 * This namespace contains AST nodes defined EXCLUSIVELY to parse target definition lang files.
 */
namespace DSL::Ast::TargetDef
{
/**
 * m_offset acts as a helper to define register ALIASES:
 * TargetRegister(rax,    ,     64, 0)
 * TargetRegister(eax,    rax,  32, 0);
 * TargetRegister(ax,     eax,  16, 0);
 * TargetRegister(ah,     ax,   8,  8);
 * TargetRegister(al,     ax,   8,  0);
 */
struct TargetRegister
{
    Common::Identifier m_name;
    Common::Identifier m_parentName;
    Common::IntegerLiteral m_size;
    Common::IntegerLiteral m_offset;
};

/**
 * TargetRegisterClass(GPR64, { TargetRegister(rax, , 64, 0), TargetRegister(rdx, , 64, 0), ... })
 */
struct TargetRegisterClass
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegister> m_registers;
};

/**
 * TargetRegisterBank(GPR, { TargetRegisterClass(GPR64), TargetRegisterClass(FPR64), ... })
 */
struct TargetRegisterBank
{
    Common::Identifier m_name;
    std::pmr::vector<TargetRegisterClass> m_classes;
};

/**
 * Node for defining an included file.
 */
struct TargetIncFile
{
    Common::Identifier m_inclusionType;
    Common::StringLiteral m_path;
};
/**
 * Node for defining the main target.
 */
struct TargetDef
{
    Common::Identifier m_name;
    std::pmr::vector<TargetIncFile> m_inclusions;
    std::pmr::vector<TargetRegisterBank> m_regBanks;
};
}; // namespace DSL::Ast::TargetDef

#endif // EZDSL_TARGET_DEF_LANG_AST_H
