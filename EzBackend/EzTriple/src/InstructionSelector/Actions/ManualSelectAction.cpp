#include "InstructionSelector/Actions/ManualSelectAction.h"

namespace SelectorActions
{
InstructionSelAction ManualAction(MirTargetInstructionId id)
{
    return [id](SelectionContext &ctx) -> SelectionResult
    {
        auto instr = *ctx.m_it;

        instr->setOpcode(MirInstructionOpCode::TARGET_INST);
        instr->setTargetId(id);

        return SelectionResult::Selected;
    };
}
}; // namespace SelectorActions
