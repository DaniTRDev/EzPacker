#include "Emitter/MirEmitter.h"

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
    }

    m_ctx = ctx;
    return true;
}

bool MirEmitter::areInstructionOperandsLegal(const MirInstructionMetadata &instructionMeta,
                                             TypedPoolLinkedList<MirOperand> *operands) const
{
    const size_t expectedBaseOperands = instructionMeta.m_operandConstraints.size();
    const std::string_view &name = instructionMeta.m_name;
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
    { return op->isOfType<MirInteger>() || op->isOfType<MirDouble>() || op->isOfType<MirConstantPoolRef>(); };

    // Validate Type Constraints per Operand
    auto it = operands->begin();
    for (size_t i = 0; i < expectedBaseOperands; ++i)
    {
        const OperandConstraint &constraint = instructionMeta.m_operandConstraints[i];
        MirOperand *op = *it;

        if (!op)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Null operand found in pool for {}", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }

        bool isValidType = false;
        ExpectedOperandType allowed = constraint.type;

        // Check against the bitmask
        if ((allowed & ExpectedOperandType::Register) && op->isOfType<MirRegister>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::Integer) && op->isOfType<MirInteger>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::Double) && op->isOfType<MirDouble>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::ConstantPoolRef) && op->isOfType<MirConstantPoolRef>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::Memory) && op->isOfType<MirMemory>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::FrameIndex) && op->isOfType<MirFrameIndex>())
            isValidType = true;
        if ((allowed & ExpectedOperandType::Reference) && op->isOfType<MirReference>())
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
        MirInstructionFlags flags = instructionMeta.m_flags;

        // Grab the first two operands using the slice iterator
        auto sizeIter = operands->begin();
        MirOperand *firstOp = *sizeIter;
        ++sizeIter;
        MirOperand *secondOp = *sizeIter;

        size_t operand1Size = firstOp->getSizeInBytes();
        size_t operand2Size = secondOp->getSizeInBytes();

        if (flags & MirInstructionFlags::SizeMatch)
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
        else if (flags & MirInstructionFlags::DestLarger)
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
        else if (flags & MirInstructionFlags::DestSmaller)
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

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode, const std::initializer_list<MirOperand *> &operands)
{
    const MirInstructionMetadata &meta = getMeta(opcode);

    // Create the base instruction
    MirInstruction *instr = emit(opcode);

    // Map the incoming operands into the instruction's TypedPoolLinkedList first
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

MirRegister *MirEmitter::createVirtualRegister(MirType *type)
{
    return m_ctx->getOperandPool()->create<MirRegister>(type, true, m_ctx->createId());
}

MirRegister *MirEmitter::createPhysicalRegister(MirType *type, size_t id)
{
    return m_ctx->getOperandPool()->create<MirRegister>(type, false, id);
}

MirInteger *MirEmitter::createImmediateInteger(MirType *type, int64_t value)
{
    // The pool returns an already-allocated, perfectly constructed MirInteger pointer.
    return m_ctx->getOperandPool()->create<MirInteger>(type, value);
}

MirDouble *MirEmitter::createImmediateDouble(MirType *type, double value)
{
    return m_ctx->getOperandPool()->create<MirDouble>(type, value);
}

MirConstantPoolRef *MirEmitter::createConstantPoolRef(MirType *type, size_t entryId)
{
    return m_ctx->getOperandPool()->create<MirConstantPoolRef>(type, entryId);
}

MirMemory *MirEmitter::createMemoryOperand(MirType *type, MirOperand *base, MirOperand *displ)
{
    return m_ctx->getOperandPool()->create<MirMemory>(type, base, displ);
}

MirFrameIndex *MirEmitter::createFrameIndex(MirType *type, size_t frameId)
{
    return m_ctx->getOperandPool()->create<MirFrameIndex>(type, frameId);
}

MirReference *MirEmitter::createReference(MirBlock *block)
{
    if (!block)
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         "Cannot create reference for null block",
                         "MirEmitterContext::createReference");
        return m_ctx->getOperandPool()->create<MirReference>(nullptr, MirReferenceType::Invalid, 0);
    }

    return m_ctx->getOperandPool()->create<MirReference>(nullptr, MirReferenceType::Block, block->getId());
}

void MirEmitter::emitOperandToInstruction(MirInstruction *instr, MirOperand *operand)
{
    emitOperands(instr, { operand });
}

bool MirEmitter::emitOperands(MirInstruction *instr, const std::initializer_list<MirOperand *> &operands)
{
    TypedPool *operandPool = m_ctx->getOperandPool();
    auto instructionOperandList = instr->getOperands();

    for (MirOperand *operand : operands)
    {
        if (!operand)
        {
            return false; // Safety check
        }

        if (!operandPool->appendToListBack<MirOperand, MirOperand>(instructionOperandList, operand))
        {
            return false;
        }
    }

    return true;
}
