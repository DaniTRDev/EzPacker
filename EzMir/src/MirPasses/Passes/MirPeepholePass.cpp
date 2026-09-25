#include "MirPasses/Passes/MirPeepholePass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include <format>

namespace
{

bool isIntegerZero(const MirOperand *op)
{
    if (!op || op->getType() != MirOperandType::Integer)
    {
        return false;
    }
    const auto *imm = op->get<MirInteger>();
    return imm && imm->getValue().isZero();
}

bool isIntegerOne(const MirOperand *op)
{
    if (!op || op->getType() != MirOperandType::Integer)
    {
        return false;
    }
    const auto *imm = op->get<MirInteger>();
    if (!imm)
    {
        return false;
    }
    const auto &val = imm->getValue();
    return val == FlexInt(1, val.getBitSize());
}

bool isIntegerAllOnes(const MirOperand *op)
{
    if (!op || op->getType() != MirOperandType::Integer)
    {
        return false;
    }
    const auto *imm = op->get<MirInteger>();
    if (!imm)
    {
        return false;
    }
    return (~imm->getValue()).isZero();
}

bool areSameRegister(const MirOperand *opA, const MirOperand *opB)
{
    if (!opA || !opB)
    {
        return false;
    }
    if (opA->getType() != MirOperandType::Register || opB->getType() != MirOperandType::Register)
    {
        return false;
    }
    const auto *regA = opA->get<MirRegister>();
    const auto *regB = opB->get<MirRegister>();
    return regA && regB && regA->getRef() == regB->getRef();
}

} // namespace

MirPeepholePass::MirPeepholePass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *MirPeepholePass::getName() const
{
    return "MirPeepholePass";
}

MirPassIterationPlace MirPeepholePass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

void MirPeepholePass::reset()
{
    m_result.reset();
}

void MirPeepholePass::printResult()
{
    if (!m_ctx || !m_ctx->getDiagCollector())
    {
        return;
    }

    if (m_ctx->getDiagCollector()->isDiagEnabledForType(Diag_Debug))
    {
        auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirPeepholePass");
        log << std::format("Peephole optimization completed: {} algebraic simplifications, {} redundant moves erased, "
                           "{} branches simplified, {} dead instructions erased.",
                           m_result.m_algebraicSimplifications,
                           m_result.m_redundantMovesEliminated,
                           m_result.m_branchesSimplified,
                           m_result.m_deadInstructionsEliminated);
    }
}

void MirPeepholePass::rewriteToMov(MirInstruction *inst, MirOperand *src)
{
    MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
    ib.swapOperand(inst, src, 1);
    if (inst->getOperandCount() >= 3)
    {
        ib.clearOperand(inst, 2);
    }
    inst->setOpcode(MirInstructionOpCode::MOV);
    m_result.m_algebraicSimplifications++;
}

void MirPeepholePass::rewriteToZero(MirInstruction *inst)
{
    MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
    MirOperandBuilder ob(m_ctx);

    MirOperand *dstOp = inst->getOperand(0);
    MirType *type = dstOp ? dstOp->getMirType() : nullptr;
    if (!type)
    {
        return;
    }

    MirOperand *zero = ob.buildInt(type, FlexInt(0, type->getTotalSizeInBits()));
    ib.swapOperand(inst, zero, 1);
    if (inst->getOperandCount() >= 3)
    {
        ib.clearOperand(inst, 2);
    }
    inst->setOpcode(MirInstructionOpCode::MOV);
    m_result.m_algebraicSimplifications++;
}

bool MirPeepholePass::trySimplifyAlgebraic(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    if (inst->isSelected() || inst->getOperandCount() < 3)
    {
        return false;
    }

    MirOperand *op0 = inst->getOperand(0);
    MirOperand *op1 = inst->getOperand(1);
    MirOperand *op2 = inst->getOperand(2);

    if (!op0 || !op1 || !op2)
    {
        return false;
    }

    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::ADD:
        {
            if (isIntegerZero(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (isIntegerZero(op1))
            {
                rewriteToMov(inst, op2);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::SUB:
        {
            if (isIntegerZero(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (areSameRegister(op1, op2))
            {
                rewriteToZero(inst);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::MUL:
        case MirInstructionOpCode::IMUL:
        {
            if (isIntegerOne(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (isIntegerOne(op1))
            {
                rewriteToMov(inst, op2);
                return true;
            }
            if (isIntegerZero(op2) || isIntegerZero(op1))
            {
                rewriteToZero(inst);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::AND:
        {
            if (isIntegerZero(op2) || isIntegerZero(op1))
            {
                rewriteToZero(inst);
                return true;
            }
            if (isIntegerAllOnes(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (isIntegerAllOnes(op1))
            {
                rewriteToMov(inst, op2);
                return true;
            }
            if (areSameRegister(op1, op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::OR:
        {
            if (isIntegerZero(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (isIntegerZero(op1))
            {
                rewriteToMov(inst, op2);
                return true;
            }
            if (areSameRegister(op1, op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::XOR:
        {
            if (isIntegerZero(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            if (isIntegerZero(op1))
            {
                rewriteToMov(inst, op2);
                return true;
            }
            if (areSameRegister(op1, op2))
            {
                rewriteToZero(inst);
                return true;
            }
            break;
        }
        case MirInstructionOpCode::SHL:
        case MirInstructionOpCode::SHR:
        {
            if (isIntegerZero(op2))
            {
                rewriteToMov(inst, op1);
                return true;
            }
            break;
        }
        default:
            break;
    }

    return false;
}

bool MirPeepholePass::trySimplifyMove(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    if (inst->getOpCode() != MirInstructionOpCode::MOV || inst->getOperandCount() < 2)
    {
        return false;
    }

    MirOperand *dst = inst->getOperand(0);
    MirOperand *src = inst->getOperand(1);

    // 1. Identity self-move: MOV %r, %r
    if (areSameRegister(dst, src))
    {
        MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
        ib.erase(inst);
        m_result.m_redundantMovesEliminated++;
        return true;
    }

    // 2. Consecutive reciprocal moves: MOV %a, %b followed by MOV %b, %a
    MirInstruction *prev = inst->getPrev();
    if (prev && prev->getOpCode() == MirInstructionOpCode::MOV && prev->getOperandCount() >= 2)
    {
        MirOperand *prevDst = prev->getOperand(0);
        MirOperand *prevSrc = prev->getOperand(1);

        if (areSameRegister(prevDst, src) && areSameRegister(prevSrc, dst))
        {
            MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
            ib.erase(inst);
            m_result.m_redundantMovesEliminated++;
            return true;
        }
    }

    return false;
}

bool MirPeepholePass::trySimplifyBranch(MirInstruction *inst, MirBlock *block)
{
    if (inst->getOpCode() != MirInstructionOpCode::JMP || inst->getOperandCount() < 1)
    {
        return false;
    }

    MirOperand *targetOp = inst->getOperand(0);
    if (!targetOp || targetOp->getType() != MirOperandType::Reference)
    {
        return false;
    }

    const auto *ref = targetOp->get<MirReference>();
    if (!ref || !ref->isBlock() || !block->getNext())
    {
        return false;
    }

    // If the target block is the next basic block in layout sequence, fall-through suffices!
    if (ref->getRefId() == block->getNext()->getId())
    {
        MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
        ib.erase(inst);
        m_result.m_branchesSimplified++;
        return true;
    }

    return false;
}

bool MirPeepholePass::eliminateDeadInstructionsAfterTerminators(MirBlock *block)
{
    bool anyDead = false;
    bool seenTerminator = false;

    for (auto it = block->getInstructions().begin(); it != block->getInstructions().end();)
    {
        MirInstruction *inst = *it;
        auto nextIt = std::next(it);

        if (seenTerminator)
        {
            MirInstructionBuilder ib(m_ctx, inst, InsertionType::InsertBefore);
            ib.erase(inst);
            m_result.m_deadInstructionsEliminated++;
            anyDead = true;
        }
        else if (inst->getFlags() & MirInstructionFlags::IsTerminator)
        {
            seenTerminator = true;
        }

        it = nextIt;
    }

    return anyDead;
}

MirPassResult MirPeepholePass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                  MirPassManager *passManager)
{
    (void)passManager;
    if (!m_ctx || !*it)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    MirFunction *func = *it;
    bool anyModified = false;
    bool changed = true;
    size_t iteration = 0;
    constexpr size_t maxIterations = 10;

    while (changed && iteration < maxIterations)
    {
        changed = false;
        iteration++;

        for (MirBlock *block : func->getBlocks())
        {
            // First prune dead instructions after any terminator in the block
            if (eliminateDeadInstructionsAfterTerminators(block))
            {
                changed = true;
                anyModified = true;
            }

            // Window scan through instructions
            for (auto instrIt = block->getInstructions().begin(); instrIt != block->getInstructions().end();)
            {
                MirInstruction *inst = *instrIt;
                auto nextIt = std::next(instrIt);

                if (trySimplifyAlgebraic(inst, block))
                {
                    changed = true;
                    anyModified = true;
                }
                else if (trySimplifyMove(inst, block))
                {
                    changed = true;
                    anyModified = true;
                }
                else if (trySimplifyBranch(inst, block))
                {
                    changed = true;
                    anyModified = true;
                }

                instrIt = nextIt;
            }
        }
    }

    return { .m_modifiedMir = anyModified, .m_executed = true, .m_succeeded = true };
}
