#include "Instruction/MirInstructionBuilder.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"

/**
 * Resolves the register information of the function owning the instruction, or nullptr when the
 * instruction is detached from any function.
 */
MirFunctionRegisterInfo *getRegInfo(MirInstruction *instr)
{
    MirBlock *block = instr->getOwner();
    if (block && block->getOwner())
    {
        return block->getOwner()->getRegisterInfo();
    }
    return nullptr;
}

/**
 * Invokes callback with the register ID of every virtual register referenced by an operand,
 * including the base and index registers of a memory operand. Null operands are ignored.
 */
static void forEachVReg(MirOperand *op, auto &&callback)
{
    if (!op)
        return;
    if (auto *reg = op->get<MirRegister>())
    {
        if (reg->isVirtual())
            callback(reg->getRegId());
    }
    else if (auto *mem = op->get<MirMemory>())
    {
        if (mem->getBase() && mem->getBase()->isVirtual())
            callback(mem->getBase()->getRegId());
        if (mem->getIndex() && mem->getIndex()->isVirtual())
            callback(mem->getIndex()->getRegId());
    }
}

/**
 * Initializes the instruction builder with parent context and insertion cursor.
 */
MirInstructionBuilder::MirInstructionBuilder(MirBuilderContext *ctx, MirInstructionInsertionPoint insertionPoint) :
    m_ctx(ctx), m_insertionPoint(std::move(insertionPoint))
{
}

/**
 * Initializes the instruction builder with target block, insertion mode, and iterator.
 */
MirInstructionBuilder::MirInstructionBuilder(MirBuilderContext *ctx,
                                             MirBlock *block,
                                             InsertionType type,
                                             IntrusiveLinkedList<MirInstruction>::iterator it) :
    m_ctx(ctx), m_insertionPoint(MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it })
{
}

/**
 * Constructs an instruction builder positioned relative to an existing instruction.
 */
MirInstructionBuilder::MirInstructionBuilder(MirBuilderContext *ctx, MirInstruction *inst, InsertionType type) :
    m_ctx(ctx),
    m_insertionPoint(
            inst && inst->getOwner()
                    ? MirInstructionInsertionPoint{ .m_type = type,
                                                    .m_block = inst->getOwner(),
                                                    .m_iterator =
                                                            inst->getOwner()->getInstructions().to_iterator(inst) }
                    : MirInstructionInsertionPoint{})
{
}

/**
 * Builds an instruction from opcode, source reference, and initializer_list of operands.
 */
MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::initializer_list<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode, ref);
    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }

    finalizeInstruction(instr, ref);
    return instr;
}

/**
 * Builds an instruction from opcode, source reference, and std::vector of operands.
 */
MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::vector<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode, ref);
    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }

    finalizeInstruction(instr, ref);
    return instr;
}

/**
 * Builds an instruction from opcode, source reference, and PMR vector of operands.
 */
MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::pmr::vector<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode, ref);

    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }

    finalizeInstruction(instr, ref);
    return instr;
}

/**
 * Builds a target machine instruction with opcode TARGET_INST and attaches the target descriptor.
 */
MirInstruction *MirInstructionBuilder::buildTarget(MirTargetInstructionDesc *targetDesc,
                                                   SourceReference *srcRef,
                                                   std::initializer_list<MirOperand *> operands)
{
    MirInstruction *instr = createInstruction(MirInstructionOpCode::TARGET_INST, srcRef);
    if (instr)
    {
        instr->setTargetDesc(targetDesc);
    }
    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }
    finalizeInstruction(instr, srcRef);
    return instr;
}

/**
 * Builds a target machine instruction with opcode TARGET_INST and attaches the target descriptor.
 */
MirInstruction *MirInstructionBuilder::buildTarget(MirTargetInstructionDesc *targetDesc,
                                                   SourceReference *srcRef,
                                                   const std::vector<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(MirInstructionOpCode::TARGET_INST, srcRef);
    if (instr)
    {
        instr->setTargetDesc(targetDesc);
    }
    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }
    finalizeInstruction(instr, srcRef);
    return instr;
}

/**
 * Builds a target machine instruction with opcode TARGET_INST and attaches the target descriptor.
 */
MirInstruction *MirInstructionBuilder::buildTarget(MirTargetInstructionDesc *targetDesc,
                                                   SourceReference *srcRef,
                                                   const std::pmr::vector<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(MirInstructionOpCode::TARGET_INST, srcRef);
    if (instr)
    {
        instr->setTargetDesc(targetDesc);
    }
    for (MirOperand *op : operands)
    {
        addOperand(instr, op);
    }
    finalizeInstruction(instr, srcRef);
    return instr;
}

/**
 * Stream operator overload for chaining and appending operands to the active instruction.
 */
MirInstructionBuilder &MirInstructionBuilder::operator<<(MirOperand *operand)
{
    if (!isBuilt() || !operand)
    {
        throw std::runtime_error("Internal Compiler Error: The instruction is not built or the operand is not valid");
    }

    m_ctx->getDiagCollector()->trace("MirInstructionBuilder",
                                     "Appended operand to inst: {}",
                                     MirPrinter::printToString(operand))
            << operand->getSourceRef();

    addOperand(getBuiltObj(), operand);
    return *this;
}

/**
 * Appends an operand to the instruction and records its def/use in the owning function's register info.
 */
MirInstructionBuilder &MirInstructionBuilder::addOperand(MirInstruction *instr, MirOperand *operand)
{
    auto &operands = instr->m_operands;
    operands.push_back(operand);
    registerOperand(instr, operand, operands.size() - 1);

    return *this;
}

/**
 * Prepends an operand, re-registering all following operands because their indices shift by one.
 */
MirInstructionBuilder &MirInstructionBuilder::addOperandFront(MirInstruction *instr, MirOperand *operand)
{
    for (size_t i = 0; i < instr->m_operands.size(); ++i)
    {
        unregisterOperand(instr, instr->m_operands[i], i);
    }

    instr->m_operands.insert(instr->m_operands.begin(), operand);

    // Re-register with shifted indices
    for (size_t i = 0; i < instr->m_operands.size(); ++i)
    {
        registerOperand(instr, instr->m_operands[i], i);
    }

    return *this;
}

/**
 * Removes every operand, unregistering their defs/uses from the function's register info.
 */
MirInstructionBuilder &MirInstructionBuilder::clearOperands(MirInstruction *instr)
{
    // Unregister all defs and uses
    for (size_t i = 0; i < instr->m_operands.size(); ++i)
    {
        unregisterOperand(instr, instr->m_operands[i], i);
    }

    instr->m_operands.clear();
    return *this;
}

/**
 * Erases the operand at pos and re-registers the shifted trailing operands with their new indices.
 */
MirInstructionBuilder &MirInstructionBuilder::clearOperand(MirInstruction *instr, size_t pos)
{
    auto &operands = instr->m_operands;
    if (pos >= operands.size())
    {
        return *this;
    }

    for (size_t i = pos; i < operands.size(); ++i)
    {
        unregisterOperand(instr, operands[i], i);
    }

    operands.erase(operands.begin() + pos);

    // Re-register remaining shifted elements
    for (size_t i = pos; i < operands.size(); ++i)
    {
        registerOperand(instr, operands[i], i);
    }

    return *this;
}

/**
 * Unregisters all operands, then unlinks the instruction from its owning block and clears its owner.
 */
MirInstructionBuilder &MirInstructionBuilder::erase(MirInstruction *instr)
{
    for (size_t i = 0; i < instr->m_operands.size(); ++i)
    {
        unregisterOperand(instr, instr->m_operands[i], i);
    }

    auto owner = instr->getOwner();
    if (owner)
    {
        owner->m_instructions.remove(instr);
        instr->m_owner = nullptr;
    }

    return *this;
}

/**
 * Replaces the operand at index, unregistering the old operand and registering the new one.
 */
MirInstructionBuilder &MirInstructionBuilder::swapOperand(MirInstruction *instr, MirOperand *newOperand, size_t index)
{
    auto &operands = instr->m_operands;

    if (index >= operands.size())
    {
        return *this;
    }

    unregisterOperand(instr, operands[index], index);
    operands[index] = newOperand;
    registerOperand(instr, newOperand, index);

    return *this;
}

/**
 * Modifies the insertion mode (Append, InsertBefore, InsertAfter) of the active insertion point.
 */
void MirInstructionBuilder::changeInsertionType(InsertionType type) { m_insertionPoint.m_type = type; }

/**
 * Sets the insertion point structure.
 */
void MirInstructionBuilder::setInsertionPoint(MirInstructionInsertionPoint insertionPoint)
{
    m_insertionPoint = std::move(insertionPoint);
}

/**
 * Configures the insertion point with target block, insertion mode, and list iterator.
 */
void MirInstructionBuilder::setInsertionPoint(MirBlock *block,
                                              InsertionType type,
                                              IntrusiveLinkedList<MirInstruction>::iterator it)
{
    m_insertionPoint = MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it };
}

/**
 * Resolves the register info from the instruction's own block, falling back to the builder's
 * insertion-point block for instructions not yet linked into a block.
 */
MirFunctionRegisterInfo *MirInstructionBuilder::getRegInfo(MirInstruction *instr) const
{
    MirBlock *block = (instr && instr->getOwner()) ? instr->getOwner() : m_insertionPoint.m_block;
    if (block && block->getOwner())
    {
        return block->getOwner()->getRegisterInfo();
    }
    return nullptr;
}

/**
 * Allocates a new unlinked MirInstruction in the arena allocator.
 */
MirInstruction *MirInstructionBuilder::createInstruction(MirInstructionOpCode opcode, SourceReference *ref)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    return alloc.new_object<MirInstruction>(m_insertionPoint.m_block,
                                            opcode,
                                            ref,
                                            std::pmr::vector<MirOperand *>(arena));
}

/**
 * Emits trace diagnostics and splices the completed instruction into the target block according to insertion type.
 */
void MirInstructionBuilder::finalizeInstruction(MirInstruction *instr, SourceReference *ref)
{
    if (!instr)
        return;

    m_ctx->getDiagCollector()->trace("MirInstructionBuilder",
                                     "Built instruction: {}",
                                     MirPrinter::printToString(instr, MirPrinterDetail::Detailed))
            << ref;

    if (m_insertionPoint.m_block)
    {
        auto &instructions = m_insertionPoint.m_block->m_instructions;

        switch (m_insertionPoint.m_type)
        {
            case InsertionType::Append:
            {
                instructions.push_back(instr);
                m_insertionPoint.m_iterator = instructions.to_iterator(instr);
                break;
            }
            case InsertionType::InsertBefore:
            {
                m_insertionPoint.m_iterator = instructions.insert(m_insertionPoint.m_iterator, instr);
                break;
            }
            case InsertionType::InsertAfter:
            {
                if (m_insertionPoint.m_iterator != instructions.end())
                {
                    auto nextIt = std::next(m_insertionPoint.m_iterator);
                    m_insertionPoint.m_iterator = instructions.insert(nextIt, instr);
                }
                else
                {
                    instructions.push_back(instr);
                    m_insertionPoint.m_iterator = instructions.to_iterator(instr);
                }
                break;
            }
        }
    }

    setBuildResult(instr);
}

/**
 * Records the operand's virtual-register def/use in the function's register info according to the
 * operand's access flag at the given index.
 */
void MirInstructionBuilder::registerOperand(MirInstruction *instr, MirOperand *op, size_t index)
{
    auto *regInfo = getRegInfo(instr);
    if (!regInfo || !op)
        return;

    MirOperandFlag flag = instr->getOperandFlag(index);
    forEachVReg(op,
                [&](MirId vregId)
                {
                    if (flag & MirOperandFlag::Write)
                        regInfo->recordDef(vregId, instr);

                    if (flag & MirOperandFlag::Read)
                        regInfo->recordUse(vregId, instr, index);
                });
}

/**
 * Removes the operand's virtual-register def/use records from the function's register info.
 */
void MirInstructionBuilder::unregisterOperand(MirInstruction *instr, MirOperand *op, size_t index)
{
    auto *regInfo = getRegInfo(instr);
    if (!regInfo || !op)
        return;

    MirOperandFlag flag = instr->getOperandFlag(index);
    forEachVReg(op,
                [&](MirId vregId)
                {
                    if (flag & MirOperandFlag::Write)
                        regInfo->clearDef(vregId);

                    if (flag & MirOperandFlag::Read)
                        regInfo->removeUse(vregId, instr);
                });
}