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
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"

/**
 * Looks up a target register class by name, building a bank/class cache on first use.
 */
MirRegisterClass *MirInstructionSelector::findClass(std::string_view name)
{
    if (!m_classLookupBuilt)
    {
        m_classLookupBuilt = true;
        m_classLookup.clear();
        if (m_targetDesc)
        {
            for (MirRegisterBank *bank : m_targetDesc->getAvailableRegisterBanks())
            {
                if (!bank)
                {
                    continue;
                }
                for (const auto &[className, regClass] : bank->getClasses())
                {
                    m_classLookup.try_emplace(className, regClass);
                }
            }
        }
    }

    auto it = m_classLookup.find(name);
    return it != m_classLookup.end() ? it->second : nullptr;
}

/**
 * Selects every block of func, returning false if any instruction could not be selected.
 */
bool MirInstructionSelector::selectFunction(MirBuilderContext *ctx, MirFunction *func)
{
    if (!ctx || !func)
        return false;

    m_currentFunction = func;

    bool allOk = true;
    bool progress = true;
    while (progress)
    {
        progress = false;
        for (MirBlock *block : func->getBlocks())
        {
            bool hasUnselected = false;
            for (MirInstruction *inst : block->getInstructions())
            {
                if (!inst->isSelected() && inst->getTargetDesc() == nullptr)
                {
                    hasUnselected = true;
                    break;
                }
            }
            if (hasUnselected)
            {
                if (!selectBlock(ctx, block))
                {
                    allOk = false;
                    return false;
                }
                progress = true;
            }
        }
    }
    return allOk;
}

/**
 * Selects all instructions in block using bottom-up maximal munch, walking backwards so
 * definitions are visited before their uses, then assigns register classes to the results.
 */
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
                ctx->getDiagCollector()->error("MirInstructionSelector",
                                               "Could not select instruction '{}'",
                                               curr->getOpCodeName())
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

/**
 * Gives unconstrained virtual registers the operand register class required by the selected
 * target instruction, defaulting to the target GPR class.
 */
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

/**
 * Asks the target addressing mode matcher to fold addrOp into a hardware addressing mode,
 * passing the selector as context so the matcher can inspect register definitions.
 */
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

/**
 * Erases every instruction absorbed by an addressing mode fold, skipping already-erased entries.
 */
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

/**
 * Returns true when reg is a virtual register with exactly one use in the current function.
 */
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

/**
 * Returns true when no instruction between from and to writes memory, has a side effect, or calls.
 * Both instructions must belong to the same (non-null) block.
 */
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
            if (flags &
                (MirInstructionFlags::WritesMemory | MirInstructionFlags::HasSideEffect | MirInstructionFlags::IsCall))
            {
                return false;
            }
        }
        else
        {
            auto flags = cur->getMetadata().m_flags;
            if (flags &
                (MirInstructionFlags::WritesMemory | MirInstructionFlags::HasSideEffect | MirInstructionFlags::IsCall))
            {
                return false;
            }
        }
        cur = cur->getNext();
    }
    return cur == to;
}

/**
 * Returns the defining instruction of a virtual register using the current function's register info.
 */
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

/**
 * Like the other overload, but falls back to scanning every function in ctx when the register's
 * definition is not visible from the currently cached function.
 */
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
