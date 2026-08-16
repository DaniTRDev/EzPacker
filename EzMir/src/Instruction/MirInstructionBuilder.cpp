#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperand.h"
#include "Printer/MirPrinter.h"

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

MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::initializer_list<MirOperand *> &operands)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirInstruction *instr = alloc.new_object<MirInstruction>(m_insertionPoint.m_block,
                                                             opcode,
                                                             ref,
                                                             std::pmr::vector<MirOperand *>(arena));
    if (instr && operands.size() != 0)
    {
        for (MirOperand *op : operands)
        {
            instr->addOperand(op);
        }
    }

    auto builder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirInstructionBuilder");
    builder << ref << "Built instruction";
    builder.appendNote(std::pmr::string(MirPrinter::printToString(instr, MirPrinterDetail::Detailed)), nullptr);

    if (instr)
    {
        auto &instructions = m_insertionPoint.m_block->getInstructions();

        if (instructions.empty())
        {
            instructions.push_back(instr);
        }
        else if (m_insertionPoint.m_type == InsertionType::InsertAfter)
        {
            // If m_iterator is instructions.end(), target the last element
            auto targetIt = (m_insertionPoint.m_iterator == instructions.end()) ? std::prev(instructions.end())
                                                                                : m_insertionPoint.m_iterator;

            // std::next(targetIt) handles inserting after the last element (becomes instructions.end())
            instructions.insert(std::next(targetIt), instr);
        }
        else // InsertionType::InsertBefore
        {
            // Standard insert before. If m_iterator == instructions.begin(),
            // it naturally inserts as the new first element.
            instructions.insert(m_insertionPoint.m_iterator, instr);
        }
    }

    setBuildResult(instr);
    return instr;
}

MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::vector<MirOperand *> &operands)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirInstruction *instr = alloc.new_object<MirInstruction>(m_insertionPoint.m_block,
                                                             opcode,
                                                             ref,
                                                             std::pmr::vector<MirOperand *>(arena));
    if (instr && !operands.empty())
    {
        for (MirOperand *op : operands)
        {
            instr->addOperand(op);
        }
    }

    auto builder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirInstructionBuilder");
    builder << ref << "Built instruction";
    builder.appendNote(std::pmr::string(MirPrinter::printToString(instr, MirPrinterDetail::Detailed)), nullptr);

    if (instr)
    {
        if (m_insertionPoint.m_type == InsertionType::InsertAfter)
        {
            auto nextIt = std::next(m_insertionPoint.m_iterator);
            m_insertionPoint.m_block->getInstructions().insert(nextIt, instr);
        }
        else
        {
            m_insertionPoint.m_block->getInstructions().insert(m_insertionPoint.m_iterator, instr);
        }
    }

    setBuildResult(instr);
    return instr;
}

MirInstruction *MirInstructionBuilder::build(MirInstructionOpCode opcode,
                                             SourceReference *ref,
                                             const std::pmr::vector<MirOperand *> &operands)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirInstruction *instr = alloc.new_object<MirInstruction>(m_insertionPoint.m_block,
                                                             opcode,
                                                             ref,
                                                             std::pmr::vector<MirOperand *>(arena));
    instr->setOperands(operands);

    auto builder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirInstructionBuilder");
    builder << ref << "Built instruction";
    builder.appendNote(std::pmr::string(MirPrinter::printToString(instr, MirPrinterDetail::Detailed)), nullptr);

    if (instr)
    {
        if (m_insertionPoint.m_type == InsertionType::InsertAfter)
        {
            auto nextIt = std::next(m_insertionPoint.m_iterator);
            m_insertionPoint.m_block->getInstructions().insert(nextIt, instr);
        }
        else
        {
            m_insertionPoint.m_block->getInstructions().insert(m_insertionPoint.m_iterator, instr);
        }
    }

    setBuildResult(instr);
    return instr;
}

MirInstruction *MirInstructionBuilder::buildTarget(MirTargetInstructionDesc *targetDesc,
                                                   SourceReference *srcRef,
                                                   std::initializer_list<MirOperand *> operands)
{
    MirInstruction *instr = build(MirInstructionOpCode::TARGET_INST, srcRef, operands);
    instr->setTargetDesc(targetDesc);

    return instr;
}

MirInstructionBuilder &MirInstructionBuilder::operator<<(MirOperand *operand)
{
    if (!isBuilt() || !operand)
    {
        throw std::runtime_error("Internal Compiler Error: The instruction is not built or the operand is not valid");
    }

    auto builder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirInstructionBuilder");
    builder << operand->getSourceRef();
    builder << "Appended operand to instruction: " << std::pmr::string(MirPrinter::printToString(operand));

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
