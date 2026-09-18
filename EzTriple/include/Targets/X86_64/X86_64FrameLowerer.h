#ifndef EZTRIPLE_X86_64_FRAME_LOWERER_H
#define EZTRIPLE_X86_64_FRAME_LOWERER_H

#include "EzTripleCommon.h"
#include "FrameLowerer/MirFrameLowerer.h"

namespace EzTriple
{

/**
 * Standard x86-64 (AMD64) frame lowering implementation.
 * Handles prologue and epilogue generation, static ALLOC stack slot mapping via LEA64r,
 * and dynamic stack frame adjustment for DALLOC.
 */
class X86_64FrameLowerer : public MirFrameLowerer
{
  public:
    X86_64FrameLowerer() = default;
    ~X86_64FrameLowerer() override = default;

    void insertPrologue(FrameLowererCtx &ctx) override;
    void insertEpilogue(FrameLowererCtx &ctx) override;
    bool lowerAlloc(FrameLowererCtx &ctx) override;
    bool lowerDAlloc(FrameLowererCtx &ctx) override;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_FRAME_LOWERER_H
