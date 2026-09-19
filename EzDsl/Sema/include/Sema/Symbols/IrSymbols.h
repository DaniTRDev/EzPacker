#ifndef EZDSLSEMA_IR_SYMBOLS_H
#define EZDSLSEMA_IR_SYMBOLS_H

#include "EzDslSemaCommon.h"
#include "SymbolCommon.h"

namespace DSL::Ast::IrInstDef
{
enum class IrInstCategory : uint8_t;
enum class IrInstFlag : uint32_t;
enum class IrInstTier : uint8_t;
enum class IrOperandDir : uint8_t;
enum class IrOperandType : uint16_t;
}; // namespace DSL::Ast::IrInstDef

namespace Symbols
{
/**
 * Resolved semantic operand of a generic IR instruction.
 */
struct IrOperandSymbol
{
    DSL::Ast::IrInstDef::IrOperandType m_type; // Accepted value kinds (bitmask).
    std::string_view m_name;                   // Operand name.
    DSL::Ast::IrInstDef::IrOperandDir m_dir;   // Dataflow direction.
};

/**
 * Resolved semantic definition of a generic IR instruction.
 */
struct IrInstructionSymbol
{
    std::string_view m_name;                        // Opcode name.
    DSL::Ast::IrInstDef::IrInstCategory m_category; // Instruction category.
    DSL::Ast::IrInstDef::IrInstTier m_tier;         // Compilation tier.
    DSL::Ast::IrInstDef::IrInstFlag m_flags;        // Combined behavioral flags.
    std::pmr::vector<IrOperandSymbol> m_operands;   // Operand signature.

    /**
     * Checks if the IR instruction has the specified flag set.
     */
    bool hasFlag(DSL::Ast::IrInstDef::IrInstFlag flagMask) const noexcept
    {
        return (static_cast<uint32_t>(m_flags) & static_cast<uint32_t>(flagMask)) != 0;
    }
};

}; // namespace Symbols

#endif // EZDSLSEMA_IR_SYMBOLS_H