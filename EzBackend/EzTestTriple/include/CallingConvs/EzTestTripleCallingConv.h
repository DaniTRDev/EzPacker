#ifndef EZPACKER_EZTESTTRIPLECALLINGCONV_H
#define EZPACKER_EZTESTTRIPLECALLINGCONV_H

#include "EzTestTripleCommon.h"
#include "RegisterBanks/EzTestTripleRegisterBanks.h"

/**
 * ============================================================================
 * Register Banks Overview & ABI Conventions
 * ============================================================================
 *
 * ----------------------------------------------------------------------------
 * Register Bank 0: GPR (General Purpose Registers)
 * Classes: GPR64 (native), GPR32 (sub), GPR16 (sub), GPR8 (sub)
 * ----------------------------------------------------------------------------
 *  ID | GPR64 | GPR32 | GPR16 | GPR8 | Role / ABI Convention
 * ----+-------+-------+-------+------+----------------------------------------
 *   0 | R0    | R0D   | R0W   | R0B  | Return Value 0 / Caller-saved
 *   1 | R1    | R1D   | R1W   | R1B  | Argument 3     / Caller-saved
 *   2 | R2    | R2D   | R2W   | R2B  | Argument 2     / Return Value 1
 *   3 | R3    | R3D   | R3W   | R3B  | General Scratch / Callee-saved
 *   4 | RSP   | -     | -     | -    | Stack Pointer (SP)
 *   5 | RFP   | -     | -     | -    | Frame Pointer (FP) / Callee-saved
 *   6 | R6    | R6D   | R6W   | R6B  | Argument 1     / Caller-saved
 *   7 | R7    | R7D   | R7W   | R7B  | Argument 0     / Caller-saved
 *   8 | R8    | R8D   | R8W   | R8B  | Argument 4     / Caller-saved
 *   9 | R9    | R9D   | R9W   | R9B  | Argument 5     / Caller-saved
 *  10 | R10   | R10D  | R10W  | R10B | Scratch        / Caller-saved
 *  11 | R11   | R11D  | R11W  | R11B | Scratch        / Caller-saved
 *  12 | R12   | R12D  | R12W  | R12B | General        / Callee-saved
 *  13 | R13   | R13D  | R13W  | R13B | General        / Callee-saved
 *  14 | R14   | R14D  | R14W  | R14B | General        / Callee-saved
 *  15 | R15   | R15D  | R15W  | R15B | General        / Callee-saved
 *
 * ----------------------------------------------------------------------------
 * Register Bank 1: FPR (Floating Point Registers)
 * Classes: FPR64 (native f64), FPR32 (sub f32)
 * ----------------------------------------------------------------------------
 *  ID | FPR64 | FPR32  | Role / ABI Convention
 * ----+-------+--------+------------------------------------------------------
 *   0 | XMM0  | XMM0_S | Float Arg 0 / Float Return 0 / Caller-saved
 *   1 | XMM1  | XMM1_S | Float Arg 1 / Caller-saved
 *   2 | XMM2  | XMM2_S | Float Arg 2 / Caller-saved
 *   3 | XMM3  | XMM3_S | Float Arg 3 / Caller-saved
 *   4 | XMM4  | XMM4_S | Float Scratch / Caller-saved
 *   5 | XMM5  | XMM5_S | Float Scratch / Caller-saved
 *   6 | XMM6  | XMM6_S | Float Scratch / Caller-saved
 *   7 | XMM7  | XMM7_S | Float Scratch / Caller-saved
 *
 * ----------------------------------------------------------------------------
 * Register Bank 2: SPR (Special Registers)
 * Classes: SPR64
 * ----------------------------------------------------------------------------
 *  ID | SPR64 | Role
 * ----+-------+--------+------------------------------------------------------
 *  0  | RIP  | Instruction pointer
 */
class EzTestTripleCallingConv : public CallingConvDesc
{
  public:
    EzTestTripleCallingConv(MirRegisterBank *gprBank, MirRegisterBank *fprBank, std::pmr::memory_resource *alloc);

    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override;
    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override;

    bool canReturnInRegs(MirType *type) const override;
    bool isCalleeCleanup() const override { return false; } // Caller clean-up
    bool doesStackGrowsDownwards() const override { return true; }
    const char *getName() const override { return "EzTestTripleCallingConv"; }

    RegisterRef getFramePointerReg() const override;
    RegisterRef getStackPointerReg() const override;

    size_t getStackAlignment() const override { return 16; }
    size_t getShadowSpaceSize() const override { return 0; }

    bool hasFramePointer(class MirFunction *func) const override;

    const std::pmr::vector<RegisterRef> &getAllCalleeSavedRegs() override;
    const std::pmr::vector<RegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override;

    const std::pmr::vector<RegisterRef> &getAllCallerSavedRegs() override;
    const std::pmr::vector<RegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override;

  private:
    MirRegisterBank *m_gprBank{ nullptr };
    MirRegisterBank *m_fprBank{ nullptr };
    std::pmr::memory_resource *m_alloc;

    // Cached Classes
    MirRegisterClass *m_gpr64{ nullptr };
    MirRegisterClass *m_gpr32{ nullptr };
    MirRegisterClass *m_gpr16{ nullptr };
    MirRegisterClass *m_gpr8{ nullptr };
    MirRegisterClass *m_fpr64{ nullptr };
    MirRegisterClass *m_fpr32{ nullptr };

    // Register Reference Cache
    std::pmr::vector<RegisterRef> m_allCalleeSaved;
    std::pmr::vector<RegisterRef> m_allCallerSaved;
    std::pmr::vector<RegisterRef> m_classCalleeSaved;
    std::pmr::vector<RegisterRef> m_classCallerSaved;

    void initRegisterLists();
    MirRegisterClass *getGprClassForSize(size_t bytes) const;
    MirRegisterClass *getFprClassForSize(size_t bytes) const;
};

#endif // EZPACKER_EZTESTTRIPLECALLINGCONV_H