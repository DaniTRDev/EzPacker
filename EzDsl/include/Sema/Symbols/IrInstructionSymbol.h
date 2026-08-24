#ifndef EZDSL_IR_INSTRUCTION_SYMBOL_H
#define EZDSL_IR_INSTRUCTION_SYMBOL_H

#include "EzDslCommon.h"
#include "Ast/IrInstructionDefLangAst.h"

namespace Sema::Symbols
{
struct IrInstructionSymbol
{
    DSL::Ast::IrInstDef::IrInstCategory m_category{ DSL::Ast::IrInstDef::IrInstCategory::Invalid };
    DSL::Ast::IrInstDef::IrInstTier m_tier{ DSL::Ast::IrInstDef::IrInstTier::HighLevel };
    DSL::Ast::IrInstDef::IrInstFlag m_flags{ DSL::Ast::IrInstDef::IrInstFlag::None };
    std::pmr::vector<DSL::Ast::IrInstDef::IrOperand> m_operands;

    /**
     * Returns true if this instruction has the given glag enabled.
     */
    bool hasFlag(DSL::Ast::IrInstDef::IrInstFlag flag) const noexcept
    {
        return (static_cast<uint32_t>(m_flags) & static_cast<uint32_t>(flag)) != 0;
    }
};
}; // namespace Sema::Symbols

#endif // EZDSL_IR_INSTRUCTION_SYMBOL_H