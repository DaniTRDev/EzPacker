#ifndef EZTARGETS_RISCV64_TARGET_INSTRUCTION_SELECTOR_H
#define EZTARGETS_RISCV64_TARGET_INSTRUCTION_SELECTOR_H

#include <riscv64InstructionSelector.h>
#include <string_view>

class MirRegisterClass;

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64 instruction selector layered on top of generated riscv64InstructionSelector,
 * overriding select() for control flow, calls, comparisons, memory, PHI and data moves.
 */
class RiscV64TargetInstructionSelector : public riscv64InstructionSelector
{
  public:
    explicit RiscV64TargetInstructionSelector(TargetDesc *targetDesc);

    bool select(MirBuilderContext *ctx, MirInstruction *inst) override;

  private:
    bool selectJMP(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectBR_COND(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectCALL(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectCMP(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectLOAD(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectSTORE(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectPHI(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectMOV(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectTRY(MirBuilderContext *ctx, MirInstruction *inst);
    bool selectRET(MirBuilderContext *ctx, MirInstruction *inst);

    TargetDesc *m_targetDesc{ nullptr };
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_TARGET_INSTRUCTION_SELECTOR_H
