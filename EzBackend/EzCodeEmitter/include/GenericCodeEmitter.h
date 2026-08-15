#ifndef EZPACKER_GENERICCODEEMITTER_H
#define EZPACKER_GENERICCODEEMITTER_H

#include "EzCodeEmitterCommon.h"

/**
 * Structure used to contain the bare minimum information about an emitted label.
 */
struct CodeLabel
{
    size_t m_id;
};

/**
 * This struct contains information about a relocation.
 */
struct Relocation
{
    MirReference *m_srcRef{ nullptr }; // Reference that caused the relocation to appear.
    uint64_t m_address{ 0 };
};

/**
 * Interface used as to contain the basic functionality needed by a code emitter to be able to emit code in a target.
 * There's an emit method for each of the possible combination of operands.
 *
 * If any error is generated, an EXCEPTION must be thrown and error information must have been pushed into the
 * diagnostic collector.
 *
 * If an instruction reaches this step, it's guaranteed that it has the correct types/values. This means that for
 * example using mov(RegisterRef, FlexInt), is a 1:1 translation that doesn't need further checks because the backend
 * has ensured the value can be moved.
 */
class GenericCodeEmitter
{
  public:
    virtual ~GenericCodeEmitter() = default;

    /**
     * Creates a code label.
     */
    virtual CodeLabel createLabel() = 0;

    /**
     * Creates a relocation out of the given reference and address.
     */
    virtual Relocation createReloc(MirReference *ref, uint64_t address) = 0;

    /**
     * Begins the emission of a function. It's name MUST BE UNIQUE across this entire emitter.
     */
    virtual void beginFunction(const std::pmr::string &name) = 0;

    /**
     * Binds the current label to the emitter, making any new emission start there.
     */
    virtual void bindLabel(const CodeLabel &label) = 0;

    /**
     * Ends the current function.
     */
    virtual void endFunction() = 0;

    // Instruction emitters.

    /**
     * Emits a single-operand ALU operation (e.g. NEG, NOT) with a register.
     */
    virtual void alur(MirTargetInstructionId opcode, const MirRegister *reg) = 0;

    /**
     * Emits an ALU (Arithmetic or Logic) operation with register, float.
     */
    virtual void alurf(MirTargetInstructionId opcode, const MirRegister *dst, const MirFloat *src) = 0;

    /**
     * Emits an ALU (Arithmetic or Logic) operation with register, integer.
     */
    virtual void aluri(MirTargetInstructionId opcode, const MirRegister *dst, const MirInteger *src) = 0;

    /**
     * Emits an ALU (Arithmetic or Logic) operation with register, register.
     */
    virtual void alurr(MirTargetInstructionId opcode, const MirRegister *dst, const MirRegister *src) = 0;

    /**
     * Emits a call and pushes a relocation to the relocation list.
     */
    virtual void calll(const CodeLabel &dst) = 0;

    /**
     * Emits a call to a register.
     */
    virtual void callr(const MirRegister *dst) = 0;

    /**
     * Emits a call to a runtime symbol and pushes a relocation.
     */
    virtual void callrt(const MirRuntimeSymbol *sym) = 0;

    /**
     * Emits a call to a reference symbol (function/global) and pushes a relocation.
     */
    virtual void callrf(const MirReference *ref) = 0;

    /**
     * Performs a cmp reg, reg.
     */
    virtual void cmprr(const MirRegister *src1, const MirRegister *src2) = 0;

    /**
     * Performs a cmp reg, float.
     */
    virtual void cmprf(const MirRegister *src1, const MirFloat *src2) = 0;

    /**
     * Performs a cmp reg, integer.
     */
    virtual void cmpri(const MirRegister *src1, const MirInteger *src2) = 0;

    /**
     * Emits a JMP to the given code label.
     */
    virtual void jmp(const CodeLabel &dest) = 0;

    /**
     * Emits a conditional jump with the given opcode and destination.
     */
    virtual void jcc(MirTargetInstructionId opcode, const CodeLabel &dest) = 0;

    /**
     * Emits a load reg, [mem]
     */
    virtual void load(const MirRegister *dst, const MirMemory *mem) = 0;

    /**
     * Emits a load reg, [ref]. The only accepted reference types are the ones that are not lowered by the
     * RelativeReferenceLowererPass. These references, in short, are the ones that need a RELOCATION.
     */
    virtual void load(const MirRegister *dst, const MirReference *ref) = 0;

    /**
     * Emits an LEA reg, [mem]
     */
    virtual void lea(const MirRegister *dst, const MirMemory *mem) = 0;

    /**
     * Emits an LEA reg, [ref]
     */
    virtual void lea(const MirRegister *dst, const MirReference *ref) = 0;

    /**
     * Emits a MOVrr.
     */
    virtual void movrr(const MirRegister *dst, const MirRegister *src) = 0;

    /**
     * Emits a MOVrf.
     */
    virtual void movrf(const MirRegister *dst, const MirFloat *src) = 0;

    /**
     * Emits a MOVri.
     */
    virtual void movri(const MirRegister *dst, const MirInteger *src) = 0;

    /**
     * Emits a pop reg.
     */
    virtual void pop(const MirRegister *dst) = 0;

    /**
     * Emits a push reg.
     */
    virtual void push(const MirRegister *src) = 0;

    /**
     * Emits a return instruction.
     */
    virtual void ret() = 0;

    /**
     * Emits a store [mem], reg
     */
    virtual void store(const MirMemory *dst, const MirRegister *src) = 0;

    /**
     * Emits a store [mem], imm
     */
    virtual void store(const MirMemory *dst, const MirInteger *src) = 0;

    /**
     * Emits a store [ref], reg. The only accepted reference types are the ones that are not lowered by the
     * RelativeReferenceLowererPass. These references, in short, are the ones that need a RELOCATION.
     */
    virtual void store(const MirReference *dst, const MirRegister *src) = 0;

    // Truncation and extension instructions.

    /**
     * Emits a zero-extend (e.g. MOVZX) from src to dst.
     */
    virtual void zext(const MirRegister *dst, const MirRegister *src) = 0;

    /**
     * Emits a sign-extend (e.g. MOVSX) from src to dst.
     */
    virtual void sext(const MirRegister *dst, const MirRegister *src) = 0;

    /**
     * Emits a truncation from src to dst.
     */
    virtual void trunc(const MirRegister *dst, const MirRegister *src) = 0;

    /**
     * Emits a floating point extend (e.g. f32 to f64) from src to dst.
     */
    virtual void fpext(const MirRegister *dst, const MirRegister *src) = 0;

    // System instructions.

    /**
     * Emits a system call instruction.
     */
    virtual void syscall() = 0;

    /**
     * Emits a NOP instruction.
     */
    virtual void nop() = 0;

    /**
     * Emits a HALT instruction.
     */
    virtual void halt() = 0;
};

#endif // EZPACKER_GENERICCODEEMITTER_H