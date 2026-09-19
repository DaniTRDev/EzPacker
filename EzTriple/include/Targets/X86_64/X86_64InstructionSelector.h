#ifndef EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H
#define EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H

#include "x86_64InstructionSelector.h"
#include <string_view>

class MirRegisterClass;

namespace EzTriple
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
    /// Looks up a register class by its declarative name, or nullptr when unknown.
    MirRegisterClass *findClass(std::string_view name);

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

    /// Lowers scalar floating-point arithmetic into SSE/AVX target instructions.
    bool selectFloatALU(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers floating-point conversions between integer and float types.
    bool selectFloatCvt(MirBuilderContext *ctx, MirInstruction *inst);

    /// Lowers a register/immediate MOV using the generated instruction table.
    bool selectMOV(MirBuilderContext *ctx, MirInstruction *inst);

    TargetDesc *m_targetDesc{ nullptr }; ///< Target descriptor owning the register classes used above.
};
} // namespace EzTriple

#endif // EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H
