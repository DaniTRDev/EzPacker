#include "InstructionSelector/Actions/SequenceAction.h"

namespace SelectorActions
{
InstructionSelAction Sequence(std::vector<MirTargetInstructionDesc *> targetDescriptors)
{
    return [descriptors = std::move(targetDescriptors)](SelectionContext &ctx) -> SelectionResult
    {
        MirInstruction *orig = *ctx.m_it;
        MirInstructionBuilder builder(ctx.m_ctx, orig->getOwner(), InsertionType::InsertBefore, ctx.m_it);

        // Emit all except the last one before the current iterator
        for (size_t i = 0; i < descriptors.size() - 1; ++i)
        {
            auto instr = builder.build(MirInstructionOpCode::TARGET_INST, orig->getSourceRef(), orig->getOperands());
            instr->setTargetDesc(descriptors[i]);
        }

        return SelectionResult::Selected;
    };
}
} // namespace SelectorActions