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

    /// Emits the standard push-rbp/mov-rbp-rsp/sub-rsp prologue, saving used callee-saved registers.
    void insertPrologue(FrameLowererCtx &ctx) override;

    /// Emits the leave/ret epilogue, restoring callee-saved registers and the stack pointer.
    void insertEpilogue(FrameLowererCtx &ctx) override;

    /// Lowers a static ALLOC into a LEA64r that materializes the stack object's address.
    bool lowerAlloc(FrameLowererCtx &ctx) override;

    /// Lowers a dynamic DALLOC by aligning RSP and copying it into the allocated pointer.
    bool lowerDAlloc(FrameLowererCtx &ctx) override;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_FRAME_LOWERER_H
