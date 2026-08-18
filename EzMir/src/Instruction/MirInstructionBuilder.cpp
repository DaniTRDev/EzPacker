#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperand.h"
#include "Printer/MirPrinter.h"
#include <stdexcept>

MirInstructionBuilder::MirInstructionBuilder(MirBuilderContext *ctx, MirInstructionInsertionPoint insertionPoint) :
    m_ctx(ctx), m_insertionPoint(std::move(insertionPoint))
{
}

MirInstructionBuilder::MirInstructionBuilder(MirBuilderContext *ctx,
                                             MirBlock *block,
                                             InsertionType type,
                                             std::pmr::list<MirInstruction *>::iterator it) :
    m_ctx(ctx), m_insertionPoint(MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it })
{
}

MirInstruction *MirInstructionBuilder::createInstruction(MirInstructionOpCode opcode, SourceReference *ref)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    return alloc.new_object<MirInstruction>(m_insertionPoint.m_block,
                                            opcode,
                                            ref,
                                            std::pmr::vector<MirOperand *>(arena));
}

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
        auto &instructions = m_insertionPoint.m_block->getInstructions();

        if (instructions.empty() || m_insertionPoint.m_type == InsertionType::Append)
        {
            instructions.push_back(instr);
        }
        else if (m_insertionPoint.m_type == InsertionType::InsertAfter)
        {
            auto targetIt = m_insertionPoint.m_iterator;
            // Advance iterator to insert *after* the target, safely guarding against end()
            if (targetIt != instructions.end())
            {
                std::advance(targetIt, 1);
            }

            instructions.insert(targetIt, instr);
        }
        else // InsertionType::InsertBefore
        {
            instructions.insert(m_insertionPoint.m_iterator, instr);
        }
    }

    setBuildResult(instr);
}

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

MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::pmr::vector<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode, ref);

    // Uses direct pmr vector assignment logic native to your instruction class
    if (!operands.empty())
    {
        instr->setOperands(operands);
    }

    finalizeInstruction(instr, ref);
    return instr;
}

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

void MirInstructionBuilder::changeInsertionType(InsertionType type) { m_insertionPoint.m_type = type; }

void MirInstructionBuilder::setInsertionPoint(MirInstructionInsertionPoint insertionPoint)
{
    m_insertionPoint = std::move(insertionPoint);
}

void MirInstructionBuilder::setInsertionPoint(MirBlock *block,
                                              InsertionType type,
                                              std::pmr::list<MirInstruction *>::iterator it)
{
    m_insertionPoint = MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it };
}