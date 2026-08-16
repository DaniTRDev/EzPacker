#include "InstructionSelector/Actions/ManualSelectAction.h"

namespace SelectorActions
{
InstructionSelAction ManualAction(MirTargetInstructionDesc *desc)
{
    return [desc](SelectionContext &ctx) -> SelectionResult
    {
        auto instr = *ctx.m_it;

        instr->setOpcode(MirInstructionOpCode::TARGET_INST);
        instr->setTargetDesc(desc);

        return SelectionResult::Selected;
    };
}
}; // namespace SelectorActions
