/**
 * @file MirEmitter.h
 * @brief High-level builder API for emitting MIR instructions into the current block.
 *
 * `MirEmitter` is a convenience layer over `MirEmitterContext`. It offers a
 * generic `emit()` entry point plus one helper per opcode generated from the
 * instruction set (`emitMOV`, `emitADD`, `emitJMP`, ...).
 *
 * The emitter validates only the operand count against instruction metadata.
 * It does not enforce deeper semantic constraints such as register-vs-memory
 * legality or size compatibility; those checks belong to later validation
 * phases.
 */
#ifndef EZPACKER_MIREMITTER_H
#define EZPACKER_MIREMITTER_H

#include "EzMirCommon.h"
#include "MirBlock.h"
#include "MirEmitterContext.h"
#include "Operand/MirOperand.h"

class MirEmitter
{
  public:
    /**
     * Creates an emitter attached to `ctx`.
     *
     * Construction fails by throwing if the emitter cannot attach to the given
     * context.
     */
    MirEmitter(MirEmitterContext *ctx);

    /**
     * Attaches the emitter to a context.
     *
     * @return `true` on success. If `ctx` is null, the method reports failure;
     *         the current implementation also attempts to report a fatal error
     *         through the context path.
     */
    bool attachToContext(MirEmitterContext *ctx);

    /**
     * Returns the context currently attached to this emitter.
     */
    MirEmitterContext *getContext() const;

    /**
     * Emits an instruction with no explicit operands.
     *
     * The created instruction is allocated by the context and, if a block is
     * currently bound there, appended to that block automatically.
     */
    MirInstruction *emit(MirInstructionOpCode opcode);

    template <typename... OperandTypes> MirInstruction *emit(MirInstructionOpCode opcode, OperandTypes &&...operands)
    {
        const size_t operandTypesSize = sizeof...(OperandTypes);
        if (operandTypesSize != getMeta(opcode).m_operandCount)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Instruction expected {} operand but got {}",
                                         getMeta(opcode).m_operandCount,
                                         operandTypesSize),
                             "MirEmitter::emit");
            return nullptr;
        }

        MirInstruction *instr = emit(opcode);
        if (operandTypesSize != 0)
        {
            if (!emitOperands(instr, std::forward<OperandTypes>(operands)...))
            {
                m_ctx->emitError(ErrorSeverity::Fatal, "Could not emit instruction operands", "MirEmitter::emit");
                return nullptr;
            }
        }

        return instr;
    }

    // Define the macro to generate a method for each instruction

#define INSTRUCTION(NAME, ARGS_COUNT, FLAGS)                                                                           \
    template <typename... OperandTypes> MirInstruction *emit##NAME(OperandTypes &&...operands)                         \
    {                                                                                                                  \
        return emit(MirInstructionOpCode::NAME, std::forward<OperandTypes>(operands)...);                              \
    }
    // Include the file again to expand the macros

#include "Instruction/MirInstructionSet.h"
#undef INSTRUCTION

    /**
     * Creates a new virtual register descriptor.
     *
     * The register receives a fresh MIR ID from the context and stores the
     * requested size in bytes.
     */
    MirRegister createRegister(size_t size);

    /**
     * Appends one operand to an existing instruction's operand slice.
     */
    void pushOperandToInstruction(MirInstruction *instr, MirOperand operand);

  private:
    /**
     * Appends one or more operands to `instr` using the context's operand pool.
     *
     * The method is recursive and returns `false` if any append operation
     * fails.
     */
    template <typename FirstOperandType, typename... OperandTypes>
    bool emitOperands(MirInstruction *instr, FirstOperandType firstOperand, OperandTypes &&...restOfOperands)
    {
        TypedPool *operandPool = m_ctx->getOperandPool();

        auto instructionOperandList = instr->getOperands();
        if (!operandPool->createAndAppendToSlice<MirOperand>(instructionOperandList, firstOperand))
        {
            return false;
        }

        if constexpr (sizeof...(OperandTypes) > 0)
        {
            return emitOperands(instr, std::forward<OperandTypes>(restOfOperands)...);
        }

        return true;
    };

  private:
    MirBlock *m_currentBlock;
    MirEmitterContext *m_ctx;
};

#endif // EZPACKER_MIREMITTER_H
