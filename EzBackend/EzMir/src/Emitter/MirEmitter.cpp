#include "Emitter/MirEmitter.h"
#include <format>

MirEmitter::MirEmitter(MirEmitterContext *ctx) : m_currentBlock(nullptr)
{
    if (!attachToContext(ctx))
    {
        throw std::runtime_error("Internal Compiler Error: Could not attach MirEmitter to context.");
    }
}

bool MirEmitter::attachToContext(MirEmitterContext *ctx)
{
    if (!ctx)
    {
        throw std::runtime_error("Could not attach MirEmitter to given context because it is null");
        return false;
    }

    m_ctx = ctx;
    return true;
}

bool MirEmitter::areInstructionOperandsLegal(const MirInstructionMetadata &instructionMeta,
                                             TypedPoolSlice<MirOperand> *operands) const
{
    const size_t expectedBaseOperands = instructionMeta.m_operands.size();
    const std::string &name = instructionMeta.m_name;
    const size_t totalOperandCount = operands->m_numElems;

    // Arity Check
    if (instructionMeta.m_opcode == MirInstructionOpCode::CALL)
    {
        if (totalOperandCount < expectedBaseOperands)
        {
            m_ctx->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Not enough operands for {}, expected at least {}", name, expectedBaseOperands),
                    "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (totalOperandCount != expectedBaseOperands)
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         std::format("Invalid operand count for {}, expected exactly {}", name, expectedBaseOperands),
                         "MirEmitter::areInstructionOperandsLegal");
        return false;
    }

    if (totalOperandCount == 0)
    {
        return true; // No operands to validate.
    }

    // Helper: Iterators return a pointer to the element in the pool.
    auto isImmediate = [](const MirOperand *op)
    { return op->getInteger() != nullptr || op->getDouble() != nullptr || op->getBigInteger() != nullptr; };

    // Validate Type Constraints per Operand
    auto it = operands->begin();
    for (size_t i = 0; i < expectedBaseOperands; ++i)
    {
        if (!it)
            break; // Defensive check, should never hit due to m_numElems validation

        const OperandConstraint &constraint = instructionMeta.m_operands[i];
        MirOperand *op = *it;

        if (!op)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Null operand found in pool for {}", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }

        bool isValidType = false;

        if ((constraint.type & ExpectedOperandType::Register) && op->getRegister())
            isValidType = true;
        if ((constraint.type & ExpectedOperandType::Immediate) && isImmediate(op))
            isValidType = true;
        if ((constraint.type & ExpectedOperandType::Reference) && op->getReference())
            isValidType = true;

        if (!isValidType)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand {} of {} has an illegal type {}, expected {}",
                                         i + 1,
                                         name,
                                         g_MirOperandType2Str[op->getType()],
                                         g_ExpectedOperandType2Str[constraint.type]),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }

        ++it;
    }

    // Validate Size Safety Rules (Requires at least 2 operands to compare)
    if (totalOperandCount >= 2)
    {
        uint32_t flags = instructionMeta.m_flags;

        // Grab the first two operands using the slice iterator
        auto sizeIter = operands->begin();
        MirOperand *firstOp = *sizeIter;
        ++sizeIter;
        MirOperand *secondOp = *sizeIter;

        size_t operand1Size = firstOp->getSizeInBytes();
        size_t operand2Size = secondOp->getSizeInBytes();

        if (flags & static_cast<uint32_t>(MirInstructionFlags::SizeMatch))
        {
            if (operand1Size != operand2Size)
            {
                m_ctx->emitError(
                        ErrorSeverity::Fatal,
                        std::format("Operand sizes in {} do not match ({} vs {})", name, operand1Size, operand2Size),
                        "MirEmitter::areInstructionOperandsLegal");
                return false;
            }
        }
        else if (flags & static_cast<uint32_t>(MirInstructionFlags::DestLarger))
        {
            if (operand1Size <= operand2Size)
            {
                m_ctx->emitError(ErrorSeverity::Fatal,
                                 std::format("Expected greater size for {}'s destination operand (Dest: {}, Src: {})",
                                             name,
                                             operand1Size,
                                             operand2Size),
                                 "MirEmitter::areInstructionOperandsLegal");
                return false;
            }
        }
        else if (flags & static_cast<uint32_t>(MirInstructionFlags::DestSmaller))
        {
            if (operand1Size >= operand2Size)
            {
                m_ctx->emitError(ErrorSeverity::Fatal,
                                 std::format("Expected smaller size for {}'s destination operand (Dest: {}, Src: {})",
                                             name,
                                             operand1Size,
                                             operand2Size),
                                 "MirEmitter::areInstructionOperandsLegal");
                return false;
            }
        }
    }

    return true;
}

MirEmitterContext *MirEmitter::getContext() const { return m_ctx; }

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode)
{
    MirInstruction *instr = m_ctx->createInstruction(opcode);
    return instr;
}

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode, const std::initializer_list<MirOperand> &operands)
{
    const MirInstructionMetadata &meta = getMeta(opcode);

    // Create the base instruction
    MirInstruction *instr = emit(opcode);

    // Map the incoming operands into the instruction's TypedPoolSlice first
    if (!emitOperands(instr, operands))
    {
        m_ctx->emitError(ErrorSeverity::Fatal, "Could not emit instruction's operands", "MirEmitter::emit");
        return nullptr;
    }

    // Now that they are in the pool as a Slice, we pass the slice to the validator
    if (!areInstructionOperandsLegal(meta, instr->getOperands()))
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         std::format("Instruction {} is illegal", meta.m_name),
                         "MirEmitter::emit");
        return nullptr;
    }

    return instr;
}

MirRegister MirEmitter::createPhysicalRegister(size_t id, size_t size)
{
    MirRegister mirRegister{ false, id, size };
    return mirRegister;
}

MirRegister MirEmitter::createVirtualRegister(size_t size)
{
    MirRegister mirRegister{ true, m_ctx->createId(), size };
    return mirRegister;
}

void MirEmitter::emitOperandToInstruction(MirInstruction *instr, MirOperand operand)
{
    emitOperands(instr, { std::move(operand) });
}

bool MirEmitter::emitOperands(MirInstruction *instr, const std::initializer_list<MirOperand> &operands)
{
    TypedPool *operandPool = m_ctx->getOperandPool();

    auto instructionOperandList = instr->getOperands();
    for (auto &operand : operands)
    {
        if (!operandPool->createAndAppendToSlice<MirOperand>(instructionOperandList, operand))
        {
            return false;
        }
    }

    return true;
}
