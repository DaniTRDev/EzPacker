#include "Instruction/MirInstructionBuilder.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Instruction/MirInstruction.h"
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
                                             IntrusiveLinkedList<MirInstruction>::iterator it) :
    m_ctx(ctx), m_insertionPoint(MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it })
{
}

MirInstruction *MirInstructionBuilder::createInstruction(MirInstructionOpCode opcode, SourceReference *ref)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator alloc(arena);

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
                                              IntrusiveLinkedList<MirInstruction>::iterator it)
{
    m_insertionPoint = MirInstructionInsertionPoint{ .m_type = type, .m_block = block, .m_iterator = it };
}