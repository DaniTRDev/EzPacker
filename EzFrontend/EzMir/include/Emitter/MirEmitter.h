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
     * Creates the emitter and attaches it to the given context. If could not attach, an exception is thrown.
     * @param ctx
     */
    MirEmitter(MirEmitterContext *ctx);

    /**
     * Attaches to the given context and returns true if succeeded. If there was any error, an exception is thrown.
     * @param ctx
     * @return bool
     */
    bool attachToContext(MirEmitterContext *ctx);

    /**
     * Returns the context this emitter is attached to.
     * @return MirEmitterContext *
     */
    MirEmitterContext *getContext() const;

    /**
     * Emits a non-operand instruction (like NOP).
     * @param opcode
     * @return MirInstruction *
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
     * Creates a virtual register of given size.
     * @return MirRegister
     */
    MirRegister createRegister(size_t size);

    /**
     * Pushes an operand to the given instruction.
     * @param instr
     * @param operand
     */
    void pushOperandToInstruction(MirInstruction *instr, MirOperand operand);

  private:
    /**
     * Emits the operands to the given instruction and returns true if succeeded.
     * @tparam FirstOperandType
     * @tparam OperandTypes
     * @return bool
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
