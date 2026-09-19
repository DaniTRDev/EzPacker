#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Descriptors/TargetDesc.h"
#include "InstructionSelector/MirAddressingModeMatcher.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"

bool MirInstructionSelector::selectFunction(MirBuilderContext *ctx, MirFunction *func)
{
    if (!ctx || !func)
        return false;

    m_currentFunction = func;

    bool allOk = true;
    for (MirBlock *block : func->getBlocks())
    {
        if (!selectBlock(ctx, block))
        {
            allOk = false;
        }
    }
    return allOk;
}

bool MirInstructionSelector::selectBlock(MirBuilderContext *ctx, MirBlock *block)
{
    if (!ctx || !block)
        return false;

    m_currentBlock = block;
    m_currentFunction = block->getOwner();

    MirInstruction *curr = block->back();
    while (curr)
    {
        MirInstruction *prev = curr->getPrev();

        if (!curr->isSelected() && curr->getTargetDesc() == nullptr)
        {
            if (!select(ctx, curr))
            {
                ctx->getDiagCollector()->error("MirInstructionSelector", "Could not select instruction '{}'", curr->getOpCodeName())
                        << curr->getSourceRef();
                return false;
            }
        }

        if (prev && !prev->isErased())
        {
            curr = prev;
        }
        else if (curr && !curr->isErased())
        {
            curr = curr->getPrev();
        }
        else
        {
            // Both curr and prev were erased (e.g. prev folded into curr, and curr replaced).
            // Scan backwards from end of block for the next unselected instruction.
            MirInstruction *scan = block->back();
            while (scan && (scan->isSelected() || scan->getTargetDesc() != nullptr))
            {
                scan = scan->getPrev();
            }
            curr = scan;
        }
    }

    // Assign required register classes to all selected instruction operands
    for (MirInstruction *inst : block->getInstructions())
    {
        if (inst && inst->isSelected())
        {
            assignRegisterClasses(inst);
        }
    }

    return true;
}

void MirInstructionSelector::assignRegisterClasses(MirInstruction *inst)
{
    if (!inst)
        return;

    const MirTargetInstructionDesc *desc = inst->getTargetDesc();
    if (!desc)
        return;

    MirRegisterClass *defaultGpr = m_targetDesc ? m_targetDesc->getGprClass() : nullptr;

    for (size_t i = 0; i < inst->getOperandCount(); ++i)
    {
        MirOperand *op = inst->getOperand(i);
        if (!op)
            continue;

        MirRegisterClass *reqClass = desc->getOperandClass(i);
        if (!reqClass)
            reqClass = defaultGpr;

        if (op->getType() == MirOperandType::Register)
        {
            auto *reg = op->get<MirRegister>();
            if (reg && reg->isVirtual() && !reg->getRegClass() && reqClass)
            {
                reg->setClass(reqClass);
            }
        }
        else if (op->getType() == MirOperandType::Memory)
        {
            auto *mem = op->get<MirMemory>();
            if (mem)
            {
                MirRegisterClass *addrClass = reqClass ? reqClass : defaultGpr;
                if (mem->getBase() && mem->getBase()->isVirtual() && !mem->getBase()->getRegClass() && addrClass)
                {
                    mem->getBase()->setClass(addrClass);
                }
                if (mem->getIndex() && mem->getIndex()->isVirtual() && !mem->getIndex()->getRegClass() && addrClass)
                {
                    mem->getIndex()->setClass(addrClass);
                }
            }
        }
    }
}

bool MirInstructionSelector::foldAddressingMode(MirBuilderContext *ctx,
                                                MirOperand *addrOp,
                                                MatchedAddressingMode &outMode,
                                                MirInstruction *rootInst)
{
    MirAddressingModeMatcher *matcher = m_targetDesc ? m_targetDesc->getAddressingModeMatcher() : nullptr;
    if (!matcher)
        return false;

    if (auto *x86Matcher = dynamic_cast<X86AddressingModeMatcher *>(matcher))
    {
        x86Matcher->setSelector(this);
        x86Matcher->setCurrentInstruction(rootInst);
    }

    return matcher->matchAddress(ctx, addrOp, outMode);
}

void MirInstructionSelector::eraseFoldedInstructions(const std::vector<MirInstruction *> &folded)
{
    for (MirInstruction *inst : folded)
    {
        if (inst && !inst->isErased())
        {
            inst->eraseFromOwner();
        }
    }
}

bool MirInstructionSelector::hasOneUse(MirRegister *reg) const
{
    if (!reg || !reg->isVirtual())
        return false;

    MirFunction *func = m_currentFunction ? m_currentFunction : (m_currentBlock ? m_currentBlock->getOwner() : nullptr);
    if (func && func->getRegisterInfo())
    {
        return func->getRegisterInfo()->hasOneUse(reg->getRegId());
    }
    return false;
}

bool MirInstructionSelector::noInterveningStore(MirInstruction *from, MirInstruction *to) const
{
    if (!from || !to)
        return false;
    if (from->getOwner() != to->getOwner() || from->getOwner() == nullptr)
        return false;

    MirInstruction *cur = from->getNext();
    while (cur && cur != to)
    {
        if (cur->isSelected() && cur->getTargetDesc())
        {
            auto flags = cur->getTargetDesc()->getTargetFlags();
            if (flags & (MirInstructionFlags::WritesMemory | MirInstructionFlags::HasSideEffect | MirInstructionFlags::IsCall))
            {
                return false;
            }
        }
        else
        {
            auto flags = cur->getMetadata().m_flags;
            if (flags & (MirInstructionFlags::WritesMemory | MirInstructionFlags::HasSideEffect | MirInstructionFlags::IsCall))
            {
                return false;
            }
        }
        cur = cur->getNext();
    }
    return cur == to;
}

MirInstruction *MirInstructionSelector::getDefiningInstruction(MirRegister *reg) const
{
    if (!reg || !reg->isVirtual())
        return nullptr;

    MirFunction *func = m_currentFunction ? m_currentFunction : (m_currentBlock ? m_currentBlock->getOwner() : nullptr);
    if (func && func->getRegisterInfo())
    {
        return func->getRegisterInfo()->getDef(reg->getRegId());
    }
    return nullptr;
}

MirInstruction *MirInstructionSelector::getDefiningInstruction(MirBuilderContext *ctx, MirRegister *reg) const
{
    if (!reg || !reg->isVirtual())
        return nullptr;

    if (auto *def = getDefiningInstruction(reg))
        return def;

    if (ctx)
    {
        for (auto func : ctx->getFunctions())
        {
            if (func->getRegisterInfo())
            {
                if (auto *def = func->getRegisterInfo()->getDef(reg->getRegId()))
                    return def;
            }
        }
    }
    return nullptr;
}
