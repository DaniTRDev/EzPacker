#include "Instruction/MirInstructionBuilder.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperand.h"
#include "Printer/MirPrinter.h"

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
 * Builds an instruction from opcode, source reference, and initializer_list of operands.
 */
MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::initializer_list<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode, ref);
    for (MirOperand *op : operands)
    {
        instr->addOperand(op);
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
        instr->addOperand(op);
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

    if (!operands.empty())
    {
        instr->setOperands(operands);
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
    MirInstruction *instr = build(MirInstructionOpCode::TARGET_INST, srcRef, operands);
    if (instr)
    {
        instr->setTargetDesc(targetDesc);
    }
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

    getBuiltObj()->addOperand(operand);
    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::addOperand(MirInstruction *instr, MirOperand *operand)
{
    instr->addOperand(operand);
    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::addOperandFront(MirInstruction *instr, MirOperand *operand)
{
    instr->m_operands.insert(instr->m_operands.begin(), operand);
    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::clearOperands(MirInstruction *instr)
{
    instr->m_operands.clear();
    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::clearOperand(MirInstruction *instr, size_t pos)
{
    auto &operands = instr->m_operands;
    if (pos >= operands.size())
    {
        return *this;
    }

    operands.erase(operands.begin() + pos);
    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::erase(MirInstruction *instr)
{
    auto owner = instr->getOwner();
    owner->m_instructions.remove(instr);

    return *this;
}

MirInstructionBuilder &MirInstructionBuilder::swapOperand(MirInstruction *instr, MirOperand *newOperand, size_t index)
{
    auto &operands = instr->m_operands;

    if (index >= operands.size())
    {
        return *this;
    }

    operands[index] = newOperand;
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