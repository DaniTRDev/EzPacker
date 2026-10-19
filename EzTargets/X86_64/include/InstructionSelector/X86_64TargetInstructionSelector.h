#ifndef EZTARGETS_X86_64_TARGET_INSTRUCTION_SELECTOR_H
#define EZTARGETS_X86_64_TARGET_INSTRUCTION_SELECTOR_H

// Angle brackets keep the search out of this header's own directory: on case-insensitive
// filesystems a quoted include would resolve "x86_64InstructionSelector.h" back to this file.
#include <x86_64InstructionSelector.h>
#include <string_view>

class MirRegisterClass;

namespace EzTargets::X86_64
{

/**
 * x86-64 pattern-matching instruction selector layered on top of the generated
 * x86_64InstructionSelector table, overriding select() to handle control flow,
 * calls, memory, PHI and floating-point instructions.
 */
class X86_64TargetInstructionSelector : public x86_64InstructionSelector
{
  public:
    /**
     * Creates the selector for the given target descriptor.
     */
    explicit X86_64TargetInstructionSelector(TargetDesc *targetDesc);

    /**
     * Dispatches one generic instruction to the matching selectSPECIALIZED helper.
     * @return True if a target pattern was matched and the instruction replaced.
     */
    bool select(MirBuilderContext *ctx, MirInstruction *inst) override;

  private:
    /// Lowers an unconditional JMP into a target jump, folding the destination if possible.
    bool selectJMP(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a conditional branch, choosing the matching Jcc condition code.
    bool selectBR_COND(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a CALL, collecting the callee reference and argument setup.
    bool selectCALL(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a comparison into a CMP plus SETcc pair materializing the boolean result.
    bool selectCMP(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a LOAD into a target memory load, folding the address computation.
    bool selectLOAD(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a STORE into a target memory store, folding the address computation.
    bool selectSTORE(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a PHI by inserting copies on the incoming edges of its block.
    bool selectPHI(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a register/immediate MOV using the generated instruction table.
    bool selectMOV(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers an integer TRUNC instruction.
    bool selectTRUNC(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers an integer SEXT instruction.
    bool selectSEXT(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers an integer ZEXT instruction.
    bool selectZEXT(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a TRY instruction into control flow to the protected body block.
    bool selectTRY(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a PUSH_REGS pseudo instruction to target stack pushes.
    bool selectPUSH_REGS(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a POP_REGS pseudo instruction to target stack pops.
    bool selectPOP_REGS(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers an integer division or remainder operation (SDIV, UDIV, REM).
    bool selectDIV(MirBuilderContext *ctx, MirInstruction *inst, bool isSigned, bool isRem);

    /// Lowers integer shift operations (SHL, SHR, SAR).
    bool selectSHIFT(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers integer subtract operation (SUB) when immediate exceeds simm32 or ISF fails.
    bool selectSUB(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers integer add operation (ADD) when immediate exceeds simm32 or ISF fails.
    bool selectADD(MirBuilderContext *ctx, MirInstruction *inst);

    TargetDesc *m_targetDesc{ nullptr }; ///< Target descriptor owning the register classes used above.
};
} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_TARGET_INSTRUCTION_SELECTOR_H
