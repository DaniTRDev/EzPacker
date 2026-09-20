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
 *
 * The flattened fields mirror the parsed declaration so non-codegen consumers (tests, future
 * passes) do not need to reach back into the AST. The current CppTargetInstructionGenerator emits
 * the opcode name and operand directions but leaves the implicit def/use slots empty until target
 * register references can be resolved; m_mnemonic is likewise retained for assembly consumers.
 */
struct TargetInstructionSymbol
{
    std::string_view m_name;                                    // Opcode name.
    std::string_view m_mnemonic;                                // Assembly mnemonic (retained; not yet emitted).
    std::pmr::vector<TargetOperandSymbol> m_operands;           // Operand signature.
    std::pmr::vector<std::string_view> m_flags;                 // Behavioral flags.
    std::pmr::vector<std::string_view> m_implicitDefs;          // Implicitly defined registers (retained; not yet emitted).
    std::pmr::vector<std::string_view> m_implicitUses;          // Implicitly used registers (retained; not yet emitted).
    std::optional<DSL::Ast::Encoding::EncodingDecl> m_encoding; // Optional generic machine encoding.
};

} // namespace Symbols

#endif // EZDSLSEMA_TARGET_SYMBOLS_H
