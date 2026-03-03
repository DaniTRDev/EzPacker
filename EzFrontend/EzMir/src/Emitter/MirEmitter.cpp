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

MirEmitterContext *MirEmitter::getContext() const { return m_ctx; }

MirInstruction *MirEmitter::emit(MirInstructionOpCode opcode)
{
    MirInstruction *instr = m_ctx->createInstruction(opcode);
    return instr;
}

MirRegister MirEmitter::createRegister(size_t size)
{
    MirRegister mirRegister{ m_ctx->createId(), size };
    return mirRegister;
}

void MirEmitter::pushOperandToInstruction(MirInstruction *instr, MirOperand operand) { emitOperands(instr, operand); }
