#ifndef EZTARGETS_RISCV64_FRAME_LOWERER_H
#define EZTARGETS_RISCV64_FRAME_LOWERER_H

#include "EzTargetsRiscV64Common.h"
#include "FrameLowerer/MirFrameLowerer.h"

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64-bit frame lowerer.
 * Handles LP64D prologue and epilogue generation, static ALLOC stack slot mapping via ADDI64ri,
 * and dynamic stack frame adjustment for DALLOC.
 */
class RiscV64FrameLowerer : public MirFrameLowerer
{
  public:
    RiscV64FrameLowerer() = default;
    ~RiscV64FrameLowerer() override = default;

    void insertPrologue(FrameLowererCtx &ctx) override;
    void insertEpilogue(FrameLowererCtx &ctx) override;
    bool lowerAlloc(FrameLowererCtx &ctx) override;
    bool lowerDAlloc(FrameLowererCtx &ctx) override;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_FRAME_LOWERER_H
