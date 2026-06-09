#include "Instruction/MirInstructionBuilder.h"

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
    std::pmr::memory_resource *arena = m_ctx->getFuncAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

    // Construct in-place, passing the arena down to the instruction's internal PMR vector
    MirInstruction *instr = alloc.new_object<MirInstruction>(opcode, ref, std::pmr::vector<MirOperand *>(arena));
    if (instr && operands.size() != 0)
    {
        for (auto &op : operands)
        {
            instr->addOperand(op);
        }
    }

    auto builder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirInstructionBuilder");
    builder << ref << "Built instruction";
    builder.appendNote(std::pmr::string(MirPrinter().printToString(instr, MirPrinterDetail::Detailed)), nullptr);

    if (instr)
    {
        if (m_insertionPoint.m_type == InsertionType::Append)
        {
            m_insertionPoint.m_block->getInstructions().push_back(instr);
        }
        else
        {
            m_insertionPoint.m_block->getInstructions().insert(m_insertionPoint.m_iterator, instr);
        }
    }

    setBuildResult(instr);
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
    builder << "Appended operand to instruction: " << std::pmr::string(MirPrinter().printToString(operand));

    getBuiltObj()->addOperand(operand);
    return *this;
}

void MirInstructionBuilder::setInsertionPoin(MirInstructionInsertionPoint insertionPoint)
{
    m_insertionPoint = std::move(insertionPoint);
}
