#ifndef EZDSLSEMA_TARGET_SYMBOLS_H
#define EZDSLSEMA_TARGET_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"
#include "Ast/EncodingDefLangAst.h"
#include "Ast/TargetInstDefLangAst.h"

namespace Symbols
{

/**
 * Resolved semantic operand of a target machine instruction.
 */
struct TargetOperandSymbol
{
    std::string_view m_regClassOrType;                     // Expected register class or operand type.
    std::string_view m_name;                               // Operand name.
    DSL::Ast::TargetInstDef::OperandDirection m_direction; // Dataflow direction.
};

/**
 * Resolved semantic definition of a target machine instruction.
 */
struct TargetInstructionSymbol
{
    std::string_view m_name;                                    // Opcode name.
    std::string_view m_mnemonic;                                // Assembly mnemonic.
    std::pmr::vector<TargetOperandSymbol> m_operands;           // Operand signature.
    std::pmr::vector<std::string_view> m_flags;                 // Behavioral flags.
    std::pmr::vector<std::string_view> m_implicitDefs;          // Implicitly defined registers.
    std::pmr::vector<std::string_view> m_implicitUses;          // Implicitly used registers.
    std::optional<DSL::Ast::Encoding::EncodingDecl> m_encoding; // Optional generic machine encoding.

    /**
     * Checks whether the instruction carries the named behavioral flag.
     */
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
