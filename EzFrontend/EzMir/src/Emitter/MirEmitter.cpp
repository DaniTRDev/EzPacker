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
        m_ctx->emitError(ErrorSeverity::Fatal,
                         "Could not create function because return type ID is null",
                         "MirEmitter::attachToContext");
        return false;
    }

    m_ctx = ctx;
    return true;
}

bool MirEmitter::areInstructionOperandsLegal(const MirInstructionMetadata &instructionMeta,
                                             MirOperand *first,
                                             MirOperand *second,
                                             size_t totalOperandCount) const
{
    if (instructionMeta.m_operandCount != totalOperandCount &&
        (instructionMeta.m_opcode == MirInstructionOpCode::CALL && totalOperandCount < 1))
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         std::format("Invalid operand count for {}", instructionMeta.m_name),
                         "MirEmitter::areInstructionOperandsLegal");
        return false;
    }

    if (totalOperandCount == 0)
    {
        // No operands, return early.
        return true;
    }

    // Safety check: ensure pointers are valid if count > 0
    if ((totalOperandCount >= 1 && !first) || (totalOperandCount >= 2 && !second))
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         std::format("Null operand passed to {}", instructionMeta.m_name),
                         "MirEmitter::areInstructionOperandsLegal");
        return false;
    }

    uint32_t flags = instructionMeta.m_flags;
    const std::string &name = instructionMeta.m_name;

    // Helper to determine if an operand is an immediate value
    auto isImmediate = [](const MirOperand *op)
    { return op->getInteger() != nullptr || op->getDouble() != nullptr || op->getBigInteger() != nullptr; };

    if (flags & static_cast<uint32_t>(MirInstructionFlags::Op1_MustBeReg))
    {
        if (!first->getRegister())
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 1 of {} must be a register", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (flags & static_cast<uint32_t>(MirInstructionFlags::Op1_MustBeMem))
    {
        if (!first->getMemory())
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 1 of {} must be memory", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (flags & static_cast<uint32_t>(MirInstructionFlags::Op1_MustBeRef))
    {
        if (!first->getReference())
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 1 of {} must be a reference", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (flags & static_cast<uint32_t>(MirInstructionFlags::Op1_MustBeRegOrImm))
    {
        if (!first->getRegister() && !isImmediate(first))
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 1 of {} must be a register or immediate", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }

    // We are guaranteed to have a second operand if these flags are present.
    if (flags & static_cast<uint32_t>(MirInstructionFlags::Op2_MustBeReg))
    {
        if (!second->getRegister())
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 2 of {} must be a register", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (flags & static_cast<uint32_t>(MirInstructionFlags::Op2_MustBeMem))
    {
        if (!second->getMemory())
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 2 of {} must be memory", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }
    else if (flags & static_cast<uint32_t>(MirInstructionFlags::Op2_MustBeRegOrImm))
    {
        if (!second->getRegister() && !isImmediate(second))
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Operand 2 of {} must be a register or immediate", name),
                             "MirEmitter::areInstructionOperandsLegal");
            return false;
        }
    }

    size_t operand1Size = first->getSize();
    size_t operand2Size = second ? second->getSize() : 0;

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
        // Changed to 'else if' for efficiency, since an instruction won't be both DestLarger and DestSmaller
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

    return true;
};

MirEmitterContext *MirEmitter::getContext() const { return m_ctx; }

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode)
{
    MirInstruction *instr = m_ctx->createInstruction(opcode);
    return instr;
}

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode, const std::initializer_list<MirOperand> &operands)
{
    const MirInstructionMetadata &meta = getMeta(opcode);
    size_t operandCount = operands.size();
    if (!areInstructionOperandsLegal(meta,
                                     (MirOperand *)operands.begin(),
                                     operandCount > 1 ? (MirOperand *)(operands.begin() + 1) : nullptr,
                                     operandCount))
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         std::format("Instruction {} is illegal", meta.m_name),
                         "MirEmitter::emit");
        return nullptr;
    }

    MirInstruction *instr = emit(opcode);
    if (!emitOperands(instr, operands))
    {
        m_ctx->emitError(ErrorSeverity::Fatal, "Could not emit instruction's operands", "MirEmitter::emit");
        return nullptr;
    }

    return instr;
}

MirRegister MirEmitter::createRegister(size_t size)
{
    MirRegister mirRegister{ m_ctx->createId(), size };
    return mirRegister;
}

void MirEmitter::pushOperandToInstruction(MirInstruction *instr, MirOperand operand)
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
