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
#include "Operand/MirOperands.h"

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
     * Checks if the operands of an instruction are legal within the MIR. For call instructions, only callee operand
     * will be checked (if it's a reference, ...). Call operands MUST BE CORRECT, so caller MUST ENSURE this on his own.
     * `totalOperandCount` is also needed to ensure before-hand that the instruction is expecting first, second, both or
     * none.
     *
     * @return `true` if the operands are legal. If they are illegal, an error is emitted. To check
     * the validity of the given operands, the metadata of the instruction is used to compare its operands to what the
     * metadata says.
     */
    bool areInstructionOperandsLegal(const MirInstructionMetadata &instructionMeta,
                                     TypedPoolLinkedList<MirOperand> *operands) const;

    /**
     * Returns the context currently attached to this emitter.
     */
    MirEmitterContext *getContext() const;

    /**
     * Emits an instruction with no explicit operands.
     *
     * The created instruction is allocated by the context and, if a block is currently bound, appended to that block
     * automatically.
     */
    MirInstruction *emit(MirInstructionOpCode opcode);

    /**
     * Emits an instruction with the given operand list.
     * @return `true` if the instruction was created and all the operands were correctly added to it, false other ways.
     * If the instruction is illegal, an error is emitted.
     */
    MirInstruction *emit(MirInstructionOpCode opcode, const std::initializer_list<MirOperand *> &operands);

    // Define the macro to generate a method for each instruction
#define INSTRUCTION(NAME, category, linearEq, ops, flags)                                                              \
    template <typename... OperandTypes> MirInstruction *emit##NAME(OperandTypes &&...operands)                         \
    {                                                                                                                  \
        std::initializer_list<MirOperand *> operandList = { std::forward<OperandTypes>(operands)... };                 \
        return emit(MirInstructionOpCode::NAME, std::move(operandList));                                               \
    } // Include the file again to expand the macros

#include "Instruction/MirInstructionSet.h"
#undef INSTRUCTION

    /**
     * Creates a physical register with the given ID and size. Id might be created with createId or not, depends on
     * target's register information.
     * @param size
     * @return
     */
    MirRegister *createPhysicalRegister(MirType *type, size_t id);

    /**
     * Creates a new virtual register descriptor.
     *
     * The register receives a fresh MIR ID from the context and stores the
     * requested size in bytes.
     */
    MirRegister *createVirtualRegister(MirType *type);

    /**
     * Creates a new reference out of the given block.
     * @param block
     * @return
     */
    MirReference *createReference(MirBlock *block);

    /**
     * Creates an immediate integer operand allocated in the context's operand pool.
     */
    MirInteger *createImmediateInteger(MirType *type, int64_t value);

    /**
     * Creates an immediate double operand allocated in the context's operand pool.
     */
    MirDouble *createImmediateDouble(MirType *type, double value);

    /**
     * Creates a constant pool reference operand allocated in the context's operand pool.
     */
    MirConstantPoolRef *createConstantPoolRef(MirType *type, size_t entryId);

    /**
     * Creates a memory access operand allocated in the context's operand pool.
     */
    MirMemory *createMemoryOperand(MirType *type, class MirOperand *base, class MirOperand *displ);

    /**
     * Creates a frame index operand allocated in the context's operand pool.
     */
    MirFrameIndex *createFrameIndex(MirType *type, size_t frameId);

    /**
     * Appends one operand to an existing instruction's operand slice.
     */
    void emitOperandToInstruction(MirInstruction *instr, MirOperand *operand);

  private:
    /**
     * Appends one or more operands to `instr` using the context's operand pool.
     *
     * The method is recursive and returns `false` if any append operation
     * fails.
     */
    bool emitOperands(MirInstruction *instr, const std::initializer_list<MirOperand *> &operands);

  private:
    MirBlock *m_currentBlock;
    MirEmitterContext *m_ctx;
};

#endif // EZPACKER_MIREMITTER_H
