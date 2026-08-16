#include "InstructionSelector/Actions/SelectRegisterClassAction.h"

namespace SelectorActions
{
InstructionSelAction SelectRegisterClass(std::vector<MirRegisterClass *> operandClasses)
{
    return [classes = std::move(operandClasses)](SelectionContext &ctx) -> SelectionResult
    {
        MirInstruction *inst = *ctx.m_it;
        if (!inst)
            return SelectionResult::SelectionError;

        const auto &operands = inst->getOperands();
        for (size_t i = 0; i < operands.size() && i < classes.size(); ++i)
        {
            MirRegisterClass *cls = classes[i];
            if (!cls)
                continue;

            MirOperand *op = operands[i];
            MirRegister *target = nullptr;

            // Direct Register
            if (op->getType() == MirOperandType::Register)
            {
                target = op->get<MirRegister>();
            }
            // Memory Operand -> extract base.
            else if (op->getType() == MirOperandType::Memory)
            {
                MirMemory *memOp = op->get<MirMemory>();
                target = memOp->getBase();
            }

            if (target)
            {
                if (target->getRef().getClass())
                {
                    ctx.m_ctx->getDiagCollector()->builder(Diag_Warning, "SelectorActions::SelectRegisterClass")
                            << "Instruction operand was already selected, overriding" << op->getSourceRef();
                }

                target->setClass(cls);
            }
        }

        return SelectionResult::Selected;
    };
}
}; // namespace SelectorActions